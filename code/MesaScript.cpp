#include "MesaScript.h"

#include "MesaCommon.h"
#include "MemoryAllocator.h"
#include "FileSystem.h"
#include "Timer.h"

#include <cmath>
#include <string>

#include "MesaScriptProfiler.cpp"
#include "MesaMath.h"

#pragma region StaticVariables

struct ProcedureDefinition
{
    std::vector<std::string> args; // todo(kevin): could just be a pointer to address in linear allocator with count
    class ASTStatementList* body;
};

NiceArray<ProcedureDefinition, PID_MAX> PROCEDURES_DATABASE;

i64 gcobjIdTicker = 1; // <= 0 is invalid
std::unordered_map<i64, MesaGCObject*> GCOBJECTS_DATABASE;

struct MesaScript_ScriptEnvironment
{
    bool KeyExistsInAccessibleScopes(const std::string &key)
    {
        // Note(Kevin): 2023-12-21 "current" scope I guess should be current function scope plus scope[0] for the script wide scope
        return scopes.back()->Contains(key) || scopes.at(0)->Contains(key);
    }

    bool KeyExistsTopLevel(const std::string &key)
    {
        return scopes.at(0)->Contains(key);
    }

    // Note(Kevin): This shouldn't perform any Contains checks. Just throw exception if doesn't exist.
    TValue AccessAtKeyInAccessibleScopes(const std::string &key)
    {
        if (scopes.back()->Contains(key))
        {
            return scopes.back()->AccessMapEntry(key);
        }
        else
        {
            return scopes.at(0)->AccessMapEntry(key);
        }
    }

    // Note(Kevin): This shouldn't perform any Contains checks. Just throw exception if doesn't exist.
    void ReplaceAtKeyInAccessibleScopes(const std::string &key, const TValue value)
    {
        if (scopes.back()->Contains(key))
        {
            return scopes.back()->ReplaceMapEntryAtKey(key, value);
        }
        else
        {
            return scopes.at(0)->ReplaceMapEntryAtKey(key, value);
        }
    }

    void EmplaceNewElement(std::string key, TValue value)
    {
        scopes.back()->CreateNewMapEntry(key, value);
    }

    /// Provided scope must have proper reference counts already (this will be done when adding map entries)
    void PushScope(MesaScript_Table *scope)
    {
        scopes.push_back(scope);
        transients.emplace_back();
    }

    /// Decrements ref counts here because we are removing a scope
    void PopScope()
    {
        if (scopes.size() > 1) // Note(Kevin): 2023-12-21 decrement ref counts only if NOT top-level script scope. We want to keep top-level script-wide GC objs alive.
            scopes.back()->DecrementReferenceCountOfEveryMapEntry();
        scopes.pop_back();
        //ASSERT(transients.back().count == 0);
        transients.pop_back();
    }

    size_t ScopesSize()
    {
        return scopes.size();
    }

    bool TransientObjectExists(i64 gcObjectId)
    {
        PLProfilerBegin(PLPROFILER_TRANSIENCY_HANDLING);
        bool exists = false;
        for (int back = int(transients.size()) - 1; back >= GM_max(0, int(transients.size()) - 2); --back)
        {
            auto &transientsAtDepth = transients.at(back);
            if (transientsAtDepth.Contains(gcObjectId))
            {
                exists = true;
                break;
            }
        }
        PLProfilerEnd(PLPROFILER_TRANSIENCY_HANDLING);
        return exists;
    }

    bool TransientObjectExistsInActiveScope(i64 gcObjectId)
    {
        return transients.back().Contains(gcObjectId);
    }

    void InsertTransientObject(i64 gcObjectId, i32 depth /*0 or negative index*/)
    {
        //ASSERT(TransientObjectExists(gcObjectId) == false);
        i32 transientsDepthIndex = (i32)transients.size() + depth - 1;
        //ASSERT(transientsDepthIndex >= 0);
        transients.at(transientsDepthIndex).PushBack(gcObjectId);
    }

    void EraseTransientObject(i64 gcObjectId)
    {
        PLProfilerBegin(PLPROFILER_TRANSIENCY_HANDLING);
        for (int back = int(transients.size()) - 1; back >= GM_max(0, int(transients.size()) - 2); --back)
        {
            auto &transientsAtDepth = transients.at(back);
            if (transientsAtDepth.Contains(gcObjectId))
            {
                transientsAtDepth.EraseFirstOf(gcObjectId);
            }
        }
        PLProfilerEnd(PLPROFILER_TRANSIENCY_HANDLING);
    }

    void ClearTransientsInLastScope()
    {
        PLProfilerBegin(PLPROFILER_TRANSIENCY_HANDLING);
        auto &lastScopeTransients = transients.back();
        for (int i = 0; i < lastScopeTransients.count;)
        {
            i64 gcobjid = lastScopeTransients.At(i);
            lastScopeTransients.EraseAt(i);
            ReleaseReferenceGCObject(gcobjid);
        }
        PLProfilerEnd(PLPROFILER_TRANSIENCY_HANDLING);
    }

private:
    std::vector<MesaScript_Table *> scopes; // TODO(Kevin): use stack allocator
    std::vector<NiceArray<i64, 16>> transients; // TODO(Kevin): use stack allocator, also "16" is a problem
};

struct MesaScriptRuntimeObject
{
    MesaScript_Table             globalEnv;
    MesaScript_ScriptEnvironment activeEnv;
};
static MesaScriptRuntimeObject __MSRuntime;
static MemoryLinearBuffer      __ASTBuffer;

static TValue returnValue;
static bool returnValueSetFlag = false;
static bool returnRequestedFlag = false;

#pragma endregion StaticVariables


enum class TokenType
{
    Default,

    LessThan,
    LessThanOrEqual,
    GreaterThan,
    GreaterThanOrEqual,
    Equal,
    NotEqual,
    LogicalAnd,
    LogicalOr,
    LogicalNegation,

    AddOperator,
    SubOperator,
    MulOperator,
    DivOperator,
    AssignmentOperator,

    NumberLiteral,
    StringLiteral,
    True,
    False,
    Identifier,

    FunctionDecl,

    LSqBrack,
    RSqBrack,

    LParen,
    RParen,
    LBrace,
    RBrace,
    Comma,

    If,
    Else,
    While,

    Return,
    EndOfLine,
    EndOfFile
};

struct Token
{
    TokenType type = TokenType::Default;
    std::string text;
    u32 startPos = 0;
    u32 line = 0;
};

void SendLexerError(const char *msg)
{
    printf("Lexer Error: %s\n", msg);
    ASSERT(0);
}

void SendCompilationError(const char* msg, Token token)
{
    printf("MesaScript Error (%d): %s\n", token.line, msg);
    ASSERT(0);
}

void SendRuntimeException(const char* msg) // TODO(Kevin): RuntimeAssert(predicate, msg)
{
    printf("Runtime exception: %s\n", msg);
    ASSERT(0); // todo do something other than halt and crash
}


MesaGCObject::GCObjectType GetTypeOfGCObject(i64 gcObjectId)
{
    MesaGCObject* gcobj = GCOBJECTS_DATABASE.at(gcObjectId);
    return gcobj->GetType();
}

MesaScript_Table* AccessMesaScriptTable(i64 gcObjectId)
{
    return (MesaScript_Table*)(GCOBJECTS_DATABASE.at(gcObjectId));
}

/// Simply returns the MesaScript_List associated with the given GCObject id.
MesaScript_List* AccessMesaScriptList(i64 gcObjectId)
{
    return (MesaScript_List*)(GCOBJECTS_DATABASE.at(gcObjectId));
}

/// Simply returns the MesaScript_String associated with the given GCObject id.
MesaScript_String* AccessMesaScriptString(i64 gcObjectId)
{
    return (MesaScript_String*)(GCOBJECTS_DATABASE.at(gcObjectId));
}

void IncrementReferenceGCObject(i64 gcObjectId)
{
    PLProfilerBegin(PLPROFILER_HASHING);
    MesaGCObject* gcobj = GCOBJECTS_DATABASE.at(gcObjectId);
    PLProfilerEnd(PLPROFILER_HASHING);
    gcobj->refCount++;

    if (__MSRuntime.activeEnv.TransientObjectExists(gcObjectId))
    {
        __MSRuntime.activeEnv.EraseTransientObject(gcObjectId);
        if (gcobj->refCount <= 0) ReleaseReferenceGCObject(gcObjectId);
    }
}

void ReleaseReferenceGCObject(i64 gcObjectId)
{
    PLProfilerBegin(PLPROFILER_HASHING);
    MesaGCObject* gcobj = GCOBJECTS_DATABASE.at(gcObjectId);
    PLProfilerEnd(PLPROFILER_HASHING);
    gcobj->refCount--;
    if (gcobj->refCount <= 0)
    {
        if (__MSRuntime.activeEnv.TransientObjectExists(gcObjectId)) return;

        if (gcobj->GetType() == MesaGCObject::GCObjectType::List)
        {
            MesaScript_List* list = AccessMesaScriptList(gcObjectId);
            list->DecrementReferenceCountOfEveryListEntry();
        }
        else if (gcobj->GetType() == MesaGCObject::GCObjectType::Table)
        {
            MesaScript_Table* map = AccessMesaScriptTable(gcObjectId);
            map->DecrementReferenceCountOfEveryMapEntry();
        }

        delete gcobj;
        PLProfilerBegin(PLPROFILER_HASHING);
        GCOBJECTS_DATABASE.erase(gcObjectId);
        PLProfilerEnd(PLPROFILER_HASHING);
    }
}

i64 RequestNewGCObject(MesaGCObject::GCObjectType gcObjectType)
{
    MesaGCObject* gcobj = NULL;
    switch(gcObjectType)
    {
        case MesaGCObject::GCObjectType::Table: {
            gcobj = (MesaGCObject*) new MesaScript_Table();
        } break;
        case MesaGCObject::GCObjectType::String: {
            gcobj = (MesaGCObject*) new MesaScript_String();
        } break;
        case MesaGCObject::GCObjectType::List: {
            gcobj = (MesaGCObject*) new MesaScript_List();
        } break;
        default: {
            SendRuntimeException("Error requesting GCObject. Undefined GCObject type.");
        } break;
    }
    gcobj->selfId = gcobjIdTicker++;
    GCOBJECTS_DATABASE.insert({gcobj->selfId, gcobj});
    return gcobj->selfId;
}

void MesaScript_List::Append(const TValue value)
{
    if (value.type == TValue::ValueType::GCObject)
    {
        IncrementReferenceGCObject(value.GCReferenceObject);
    }

    list.push_back(value);
}

void MesaScript_List::ReplaceListEntryAtIndex(const i64 index, const TValue value)
{
    TValue existingValue = list.at(index);
    if (existingValue.type == TValue::ValueType::GCObject)
    {
        if (value.type == TValue::ValueType::GCObject && existingValue.GCReferenceObject == value.GCReferenceObject)
        {
            return;
        }
        else
        {
            ReleaseReferenceGCObject(existingValue.GCReferenceObject);
        }
    }

    if (value.type == TValue::ValueType::GCObject)
    {
        IncrementReferenceGCObject(value.GCReferenceObject);        
    }

    list.at(index) = value;
}

void MesaScript_List::DecrementReferenceCountOfEveryListEntry()
{
    for (int i = 0, size = (int)list.size(); i < size; ++i)
    {
        TValue v = list.at(i);
        if (v.type == TValue::ValueType::GCObject)
        {
            ReleaseReferenceGCObject(v.GCReferenceObject);
        }
    }
}

bool MesaScript_Table::Contains(const std::string &key)
{
    PLProfilerBegin(PLPROFILER_HASHING);
    auto elemIterator = table.find(key);
    bool v = elemIterator != table.end();
    PLProfilerEnd(PLPROFILER_HASHING);
    return v;
}

TValue MesaScript_Table::AccessMapEntry(const std::string &key)
{
    PLProfilerBegin(PLPROFILER_HASHING);
    TValue value = table.at(key);
    PLProfilerEnd(PLPROFILER_HASHING);
    return value;
}

void MesaScript_Table::CreateNewMapEntry(const std::string& key, const TValue value)
{
    if (value.type == TValue::ValueType::GCObject)
    {
        IncrementReferenceGCObject(value.GCReferenceObject);
    }
    PLProfilerBegin(PLPROFILER_HASHING);
    table.emplace(key, value);
    PLProfilerEnd(PLPROFILER_HASHING);
}

void MesaScript_Table::ReplaceMapEntryAtKey(const std::string& key, const TValue value)
{
    PLProfilerBegin(PLPROFILER_HASHING);
    TValue existingValue = table.at(key);
    PLProfilerEnd(PLPROFILER_HASHING);
    if (existingValue.type == TValue::ValueType::GCObject)
    {
        if (value.type == TValue::ValueType::GCObject && existingValue.GCReferenceObject == value.GCReferenceObject)
        {
            return;
        }
        else
        {
            ReleaseReferenceGCObject(existingValue.GCReferenceObject);
        }
    }

    if (value.type == TValue::ValueType::GCObject)
    {
        IncrementReferenceGCObject(value.GCReferenceObject);        
    }

    PLProfilerBegin(PLPROFILER_HASHING);
    table.at(key) = value;
    PLProfilerEnd(PLPROFILER_HASHING);
}

void MesaScript_Table::DecrementReferenceCountOfEveryMapEntry()
{
    for (const auto& pair : table)
    {
        TValue v = pair.second;
        if (v.type == TValue::ValueType::GCObject)
        {
            ReleaseReferenceGCObject(v.GCReferenceObject);
        }
    }
}

int ActiveScopeDepthIndex()
{
    return (int) __MSRuntime.activeEnv.ScopesSize() - 1;
}

MesaScript_Table* EmplaceMapInGlobalScope(const std::string& id)
{
    TValue v;
    v.type = TValue::ValueType::GCObject;
    v.GCReferenceObject = RequestNewGCObject(MesaGCObject::GCObjectType::Table);
    __MSRuntime.globalEnv.CreateNewMapEntry(id, v);
    return (MesaScript_Table*) GCOBJECTS_DATABASE.at(v.GCReferenceObject);
}

MesaScript_Table* AccessMapInGlobalScope(const std::string& id)
{
    TValue v = __MSRuntime.globalEnv.AccessMapEntry(id);
    return (MesaScript_Table*) GCOBJECTS_DATABASE.at(v.GCReferenceObject);
}

#pragma region Lexer

bool IsValueType(TokenType type)
{
    return
            type == TokenType::NumberLiteral ||
            type == TokenType::Identifier ||
            type == TokenType::RParen;
}

// char -> bool
// return true if char is one of [0, 9]
bool IsDigit(char c)
{
    return ('0' <= c && c <= '9');
}

bool IsCharacter(char c)
{
    return
            ('a' <= c && c <= 'z') ||
            ('A' <= c && c <= 'Z') ||
            ('_' == c);
}

bool IsWhitespace(char c)
{
    return c == ' ' /* or some other shit*/;
}

// string -> list of Token
std::vector<Token> Lexer(const std::string& code)
{
    std::vector<Token> retval;
    u32 currentIndex = 0;
    u32 currentLine = 1;

    bool flag_NegativeNumberAhead = false;
    while(currentIndex < code.length())
    {
        u32 tokenStartIndex = currentIndex;
        char lookAhead = code.at(currentIndex);
        if(IsWhitespace(lookAhead))
        {
            ++currentIndex;
        }
        else if(lookAhead == '"' || lookAhead == '\'')
        {
            char quotationInUse = lookAhead == '"' ? '"' : '\'';
            ++currentIndex;
            while (currentIndex < (code.length() - 1) && code.at(currentIndex) != quotationInUse)
            {
                ++currentIndex;
            }
            Token token;
            token.type = TokenType::StringLiteral;
            token.text = code.substr(tokenStartIndex + 1, currentIndex - (tokenStartIndex + 1));
            token.startPos = tokenStartIndex;
            token.line = currentLine;
            ++currentIndex;
            retval.push_back(token);
        }
        else if(lookAhead == '-')
        {
            ++currentIndex;
            if (!IsValueType(retval.back().type) && currentIndex < code.length() && IsDigit(code.at(currentIndex)))
            {
                flag_NegativeNumberAhead = true;
            }
            else
            {
                retval.push_back({ TokenType::SubOperator, code.substr(tokenStartIndex, currentIndex - tokenStartIndex), tokenStartIndex, currentLine });
            }
        }
        else if (lookAhead == '`') // block comments
        {
            ++currentIndex;
            while (currentIndex < (code.length() - 1) && code.at(currentIndex) != '`')
            {
                if (code.at(currentIndex) == '\n') ++currentLine;
                ++currentIndex;
            }
            ++currentIndex;
        }
        else if (lookAhead == '~' || lookAhead == ';') // line comments
        {
            ++currentIndex;
            while (currentIndex < (code.length() - 1) && code.at(currentIndex) != '\n')
            {
                ++currentIndex;
            }
            ++currentIndex;
            ++currentLine;
        }
        else if(lookAhead == '<' || lookAhead == '>' || lookAhead == '!' || lookAhead == '=')
        {
            ++currentIndex;
            if (currentIndex < code.length() && code.at(currentIndex) == '=') // check if next char is =
            {
                ++currentIndex;
                if (lookAhead == '<')
                {
                    retval.push_back({ TokenType::LessThanOrEqual, code.substr(tokenStartIndex, currentIndex - tokenStartIndex), tokenStartIndex, currentLine });
                }
                else if (lookAhead == '>')
                {
                    retval.push_back({ TokenType::GreaterThanOrEqual, code.substr(tokenStartIndex, currentIndex - tokenStartIndex), tokenStartIndex, currentLine });
                }
                else if (lookAhead == '!')
                {
                    retval.push_back({ TokenType::NotEqual, code.substr(tokenStartIndex, currentIndex - tokenStartIndex), tokenStartIndex, currentLine });
                }
                else if (lookAhead == '=')
                {
                    retval.push_back({ TokenType::Equal, code.substr(tokenStartIndex, currentIndex - tokenStartIndex), tokenStartIndex, currentLine });
                }
            }
            else if (lookAhead == '<')
            {
                retval.push_back({ TokenType::LessThan, code.substr(tokenStartIndex, currentIndex - tokenStartIndex), tokenStartIndex, currentLine });
            }
            else if (lookAhead == '>')
            {
                retval.push_back({ TokenType::GreaterThan, code.substr(tokenStartIndex, currentIndex - tokenStartIndex), tokenStartIndex, currentLine });
            }
            else if (lookAhead == '=')
            {
                retval.push_back({ TokenType::AssignmentOperator, code.substr(tokenStartIndex, currentIndex - tokenStartIndex), tokenStartIndex, currentLine });
            }
            else if (lookAhead == '!')
            {
                retval.push_back({TokenType::LogicalNegation, code.substr(tokenStartIndex, currentIndex - tokenStartIndex), tokenStartIndex, currentLine });
            }
        }
        else if(IsDigit(lookAhead))
        {
            bool bDecimalPointEncountered = false;
            if (flag_NegativeNumberAhead)
            {
                tokenStartIndex -= 1;
                flag_NegativeNumberAhead = false;
            }
            while (currentIndex < code.length() && 
                   (IsDigit(code.at(currentIndex)) || (!bDecimalPointEncountered && code.at(currentIndex) == '.')))
            {
                if (code.at(currentIndex) == '.') 
                {
                    ASSERT(currentIndex + 1 < code.length());
                    ASSERT(IsDigit(code.at(currentIndex + 1)));
                    bDecimalPointEncountered = true;
                }
                ++currentIndex;
            }
            retval.push_back({ TokenType::NumberLiteral, code.substr(tokenStartIndex, currentIndex - tokenStartIndex), tokenStartIndex, currentLine });
        }
        else if(IsCharacter(lookAhead))
        {
            NiceArray<char, 65> wordCharsBuffer; // then collect string of character to form word
            wordCharsBuffer.ResetToZero();
            while(currentIndex < code.length() && (IsCharacter(code.at(currentIndex)) || IsDigit(code.at(currentIndex))))
            {
                wordCharsBuffer.PushBack(code.at(currentIndex));
                ++currentIndex;
            } // at this point, we have a fully formed word

            auto word = std::string(wordCharsBuffer.data);
            if (word == "return")
            {
                retval.push_back({ TokenType::Return, code.substr(tokenStartIndex, currentIndex - tokenStartIndex), tokenStartIndex, currentLine });
            }
            else if (word == "true")
            {
                retval.push_back({ TokenType::True, code.substr(tokenStartIndex, currentIndex - tokenStartIndex), tokenStartIndex, currentLine });
            }
            else if (word == "false")
            {
                retval.push_back({ TokenType::False, code.substr(tokenStartIndex, currentIndex - tokenStartIndex), tokenStartIndex, currentLine });
            }
            else if (word == "and")
            {
                retval.push_back({TokenType::LogicalAnd, code.substr(tokenStartIndex, currentIndex - tokenStartIndex), tokenStartIndex, currentLine });
            }
            else if (word == "or")
            {
                retval.push_back({TokenType::LogicalOr, code.substr(tokenStartIndex, currentIndex - tokenStartIndex), tokenStartIndex, currentLine });
            }
            else if (word == "if")
            {
                retval.push_back({ TokenType::If, code.substr(tokenStartIndex, currentIndex - tokenStartIndex), tokenStartIndex, currentLine });
            }
            else if (word == "else")
            {
                retval.push_back({ TokenType::Else, code.substr(tokenStartIndex, currentIndex - tokenStartIndex), tokenStartIndex, currentLine });
            }
            else if (word == "fn")
            {
                retval.push_back({ TokenType::FunctionDecl, code.substr(tokenStartIndex, currentIndex - tokenStartIndex), tokenStartIndex, currentLine });
            }
            else if (word == "while")
            {
                retval.push_back({ TokenType::While, code.substr(tokenStartIndex, currentIndex - tokenStartIndex), tokenStartIndex, currentLine });
            }
            else // otherwise, word is function call or identifier
            {
                retval.push_back({ TokenType::Identifier, code.substr(tokenStartIndex, currentIndex - tokenStartIndex), tokenStartIndex, currentLine });
            }
        }
        else
        {
            ++currentIndex;
            TokenType tokenType;
            switch(lookAhead)
            {
                case '+': { tokenType = TokenType::AddOperator; } break;
                case '*': { tokenType = TokenType::MulOperator; } break;
                case '/': { tokenType = TokenType::DivOperator; } break;
                case '(': { tokenType = TokenType::LParen; } break;
                case ')': { tokenType = TokenType::RParen; } break;
                case '{': { tokenType = TokenType::LBrace; } break;
                case '}': { tokenType = TokenType::RBrace; } break;
                case '[': { tokenType = TokenType::LSqBrack; } break;
                case ']': { tokenType = TokenType::RSqBrack; } break;
                case ',': { tokenType = TokenType::Comma; } break;
                case '\n': { ++currentLine; continue; /*tokenType = TokenType::EndOfLine;*/ } break;
                case '\r': { continue; /*tokenType = TokenType::EndOfLine;*/ } break;
                default:{
                    SendLexerError("error: unrecognized character in Lexer\n");
                    continue;
                }
            }
            retval.push_back({ tokenType, code.substr(tokenStartIndex, currentIndex - tokenStartIndex), tokenStartIndex, currentLine });
        }
    }
    retval.push_back({TokenType::EndOfFile, "<EOF>", (u32)code.length(), currentLine });
    return retval;
}

#pragma endregion Lexer

#pragma region ASTAndParser

enum class ASTNodeType
{
    STATEMENTLIST,
    ASSIGN,
    VARIABLE,
    PROCEDURECALL,
    RETURN,
    WHILE,
    NUMBER,
    STRING,
    BOOLEAN,
    BINOP,
    RELOP,
    LOGICALNOT,
    BRANCH,
    CREATETABLE,
    CREATELIST,
    ASSIGN_LIST_OR_MAP_ELEMENT,
    ACCESS_LIST_OR_MAP_ELEMENT,

    SIMPLYTVALUE
};

class ASTNode
{
public:
    ASTNode(ASTNodeType type);
    inline ASTNodeType GetType() { return nodeType; }
private:
    ASTNodeType nodeType;
};

class ASTStatementList : public ASTNode
{
public:
    ASTStatementList();

    std::vector<ASTNode*> statements;
};

class ASTAssignment : public ASTNode
{
public:
    ASTAssignment(ASTNode* id, ASTNode* expr);

    ASTNode* id;
    ASTNode* expr;
};

class ASTVariable : public ASTNode
{
public:
    ASTVariable(const std::string& id);

    std::string id;
};

class ASTProcedureCall : public ASTNode
{
public:
    ASTProcedureCall(const std::string& id);

    std::string id;
    std::vector<ASTNode*> argsExpressions;
};

class ASTReturn : public ASTNode
{
public:
    ASTReturn(ASTNode* expr);

    ASTNode* expr;
};

class ASTCreateTable : public ASTNode
{
public:
    ASTCreateTable()
        : ASTNode(ASTNodeType::CREATETABLE)
    {}

    //std::vector<ASTNode*> tableInitializingEntries;
};

class ASTCreateList : public ASTNode
{
public:
    ASTCreateList()
        : ASTNode(ASTNodeType::CREATELIST)
    {}

    std::vector<ASTNode*> listInitializingElements;
};

class ASTAssignListOrMapElement : public ASTNode
{
public:
    ASTAssignListOrMapElement(ASTNode* id, ASTNode* indexExpr, ASTNode* valueExpr)
        : ASTNode(ASTNodeType::ASSIGN_LIST_OR_MAP_ELEMENT)
        , listOrMapVariableName(id)
        , indexExpression(indexExpr)
        , valueExpression(valueExpr)
    {}

    ASTNode* listOrMapVariableName;
    ASTNode* indexExpression;
    ASTNode* valueExpression;
};

class ASTAccessListOrMapElement : public ASTNode
{
public:
    ASTAccessListOrMapElement(ASTNode* id, ASTNode* indexExpr)
        : ASTNode(ASTNodeType::ACCESS_LIST_OR_MAP_ELEMENT)
        , listOrMapVariableName(id)
        , indexExpression(indexExpr)
    {}

    ASTNode* listOrMapVariableName;
    ASTNode* indexExpression;
};

class ASTWhile : public ASTNode
{
public:
    ASTWhile(ASTNode *condition, ASTNode *body);

    ASTNode* condition;
    ASTNode* body;
};

class ASTNumberTerminal : public ASTNode
{
public:
    ASTNumberTerminal(double num, bool integer);

    double value;
    bool isInteger = false;
};

class ASTStringTerminal : public ASTNode
{
public:
    ASTStringTerminal(std::string str);

    std::string value;
};

class ASTBooleanTerminal : public ASTNode
{
public:
    ASTBooleanTerminal(bool v);

    bool value;
};

class ASTSimplyTValue : public ASTNode
{
public:
    ASTSimplyTValue(TValue v);

    TValue value;
};

enum class BinOp
{
    Add,
    Sub,
    Mul,
    Div
};

class ASTBinOp : public ASTNode
{
public:
    ASTBinOp(BinOp op, ASTNode* left, ASTNode* right);

    BinOp op;
    ASTNode* left;
    ASTNode* right;
};

enum class RelOp
{
    LT,
    GT,
    LE,
    GE,
    EQ,
    NEQ,
    AND, // and logical AND OR
    OR
};

class ASTRelOp : public ASTNode
{
public:
    ASTRelOp(RelOp op, ASTNode* left, ASTNode* right);

    RelOp op;
    ASTNode* left;
    ASTNode* right;
};

class ASTLogicalNot : public ASTNode
{
public:
    ASTLogicalNot(ASTNode* boolExpr);

    ASTNode* boolExpr;
};

class ASTBranch : public ASTNode
{
public:
    ASTBranch(ASTNode* condition, ASTNode* if_case, ASTNode* else_case);

    ASTNode* condition;
    ASTNode* if_body;
    ASTNode* else_body;
};

ASTNode::ASTNode(ASTNodeType type)
    : nodeType(type)
{}

ASTStatementList::ASTStatementList()
    : ASTNode(ASTNodeType::STATEMENTLIST)
{}

ASTAssignment::ASTAssignment(ASTNode* id, ASTNode* expr)
    : ASTNode(ASTNodeType::ASSIGN)
    , id(id)
    , expr(expr)
{}

ASTVariable::ASTVariable(const std::string& id)
    : ASTNode(ASTNodeType::VARIABLE)
    , id(id)
{}

ASTProcedureCall::ASTProcedureCall(const std::string& id)
        : ASTNode(ASTNodeType::PROCEDURECALL)
        , id(id)
{}

ASTReturn::ASTReturn(ASTNode* expr)
    : ASTNode(ASTNodeType::RETURN)
    , expr(expr)
{}

ASTWhile::ASTWhile(ASTNode *condition, ASTNode *body)
    : ASTNode(ASTNodeType::WHILE)
    , condition(condition)
    , body(body)
{}

ASTNumberTerminal::ASTNumberTerminal(double num, bool integer)
    : ASTNode(ASTNodeType::NUMBER)
    , value(num)
    , isInteger(integer)
{}

ASTStringTerminal::ASTStringTerminal(std::string str)
    : ASTNode(ASTNodeType::STRING)
    , value(str)
{}

ASTBooleanTerminal::ASTBooleanTerminal(bool v)
    : ASTNode(ASTNodeType::BOOLEAN)
    , value(v)
{}

ASTBinOp::ASTBinOp(BinOp op, ASTNode* left, ASTNode* right)
    : ASTNode(ASTNodeType::BINOP)
    , op(op)
    , left(left)
    , right(right)
{}

ASTRelOp::ASTRelOp(RelOp op, ASTNode* left, ASTNode* right)
    : ASTNode(ASTNodeType::RELOP)
    , op(op)
    , left(left)
    , right(right)
{}

ASTLogicalNot::ASTLogicalNot(ASTNode* boolExpr)
    : ASTNode(ASTNodeType::LOGICALNOT)
    , boolExpr(boolExpr)
{}

ASTBranch::ASTBranch(ASTNode* condition, ASTNode* if_case, ASTNode* else_case)
        : ASTNode(ASTNodeType::BRANCH)
        , condition(condition)
        , if_body(if_case)
        , else_body(else_case)
{}

ASTSimplyTValue::ASTSimplyTValue(TValue v)
    : ASTNode(ASTNodeType::SIMPLYTVALUE)
    , value(v)
{}

class Parser
{
public:
    Parser(std::vector<Token> _tokens);

    void parse();

private:
    void eat(TokenType tpe);

    ASTNode* procedure_call();
    ASTNode* factor();
    ASTNode* term();
    ASTNode* expr();

    ASTNode* cond_expr();
    ASTNode* cond_equal();
    ASTNode* cond_and();
    ASTNode* cond_or();
    ASTNode* table_or_list_or_cond_or();

    ASTNode* statement();
    ASTStatementList* statement_list();
    PID procedure_decl();


private:
    std::vector<Token> tokens;
    Token currentToken;
    size_t currentTokenIndex;

public:
    //std::vector<ASTProcedureCall *> temporaryProcedureExecutionQueue;
    std::vector<ASTNode*> scriptExecutionQueue;
};

Parser::Parser(std::vector<Token> _tokens)
        : tokens(_tokens)
        , currentToken(tokens.front())
        , currentTokenIndex(0)
{}

void Parser::parse()
{
    while (currentToken.type != TokenType::EndOfFile)
    {
        if (currentToken.type == TokenType::FunctionDecl)
        {
            procedure_decl();
        }
        else
        {
            scriptExecutionQueue.push_back(statement());
        }
    }
}

PID Parser::procedure_decl()
{
    eat(TokenType::FunctionDecl);
    ASSERT(currentToken.type == TokenType::Identifier);
    auto procedureNameToken = currentToken;
    eat(TokenType::Identifier);
    eat(TokenType::LParen);

    PROCEDURES_DATABASE.PushBack(ProcedureDefinition());
    const PID createdProcedureId = PROCEDURES_DATABASE.count - 1;
    auto& argsVector = PROCEDURES_DATABASE.At((unsigned int)createdProcedureId).args;

    while(currentToken.type != TokenType::RParen)
    {
        argsVector.push_back(currentToken.text);
        eat(TokenType::Identifier);
        if (currentToken.type != TokenType::RParen) eat(TokenType::Comma);
    }
    eat(TokenType::RParen);

    PROCEDURES_DATABASE.At((unsigned int)createdProcedureId).body = statement_list();

    TValue functionVariable;
    functionVariable.procedureId = createdProcedureId;
    functionVariable.type = TValue::ValueType::Function;

    if (__MSRuntime.activeEnv.KeyExistsTopLevel(procedureNameToken.text))
    {
        // Maybe throw warning since it overwrites previously declared identifier in the top level
        __MSRuntime.activeEnv.ReplaceAtKeyInAccessibleScopes(procedureNameToken.text, functionVariable);
    }
    else
    {
        __MSRuntime.activeEnv.EmplaceNewElement(procedureNameToken.text, functionVariable);
    }
    return createdProcedureId;
}

ASTNode* Parser::procedure_call()
{
    auto procSymbol = currentToken;
    eat(TokenType::Identifier);

    auto node =
            new (MemoryLinearAllocate(&__ASTBuffer, sizeof(ASTProcedureCall), alignof(ASTProcedureCall)))
                    ASTProcedureCall(procSymbol.text);

    eat(TokenType::LParen);

    while(currentToken.type != TokenType::RParen)
    {
        node->argsExpressions.push_back(cond_or());
        if (currentToken.type != TokenType::RParen) eat(TokenType::Comma);
    }

    eat(TokenType::RParen);

    return node;
}

ASTStatementList* Parser::statement_list()
{
    auto statement_list = new (MemoryLinearAllocate(&__ASTBuffer, sizeof(ASTStatementList), alignof(ASTStatementList)))
            ASTStatementList();
    eat(TokenType::LBrace);
    while(currentToken.type != TokenType::RBrace)
    {
        statement_list->statements.push_back(statement());
    }
    eat(TokenType::RBrace);
    return statement_list;
}

ASTNode* Parser::table_or_list_or_cond_or()
{
    ASTNode* value = nullptr;
    if (currentToken.type == TokenType::LBrace)
    {
        eat(TokenType::LBrace);
        // TODO: initialize map with entries
        eat(TokenType::RBrace);

        value = new (MemoryLinearAllocate(&__ASTBuffer, sizeof(ASTCreateTable), alignof(ASTCreateTable)))
            ASTCreateTable();
    }
    else if (currentToken.type == TokenType::LSqBrack)
    {
        value = new (MemoryLinearAllocate(&__ASTBuffer, sizeof(ASTCreateList), alignof(ASTCreateList)))
                    ASTCreateList();
        ASTCreateList* valueCastedToCreateListPtr = static_cast<ASTCreateList*>(value);

        eat(TokenType::LSqBrack);
        while(currentToken.type != TokenType::RSqBrack)
        {
            ASTNode* elementInitializerResult = table_or_list_or_cond_or();
            valueCastedToCreateListPtr->listInitializingElements.push_back(elementInitializerResult);
            if (currentToken.type != TokenType::RSqBrack) eat(TokenType::Comma);
        }
        eat(TokenType::RSqBrack);
    }
    else
    {
        value = cond_or();
    }
    return value;
}

ASTNode* Parser::statement()
{
    if (currentToken.type == TokenType::Identifier)
    {
        auto nextToken = tokens.at(currentTokenIndex + 1);
        if (nextToken.type == TokenType::LParen)
        {
            return procedure_call();
        }
        else if (nextToken.type == TokenType::LSqBrack)
        {
            // map["x"] = ? | list[0] = ? | listOrMap[i] = ?
            auto t = currentToken;

            eat(TokenType::Identifier);
            eat(TokenType::LSqBrack);

            auto indexExpr = expr();

            eat(TokenType::RSqBrack);
            eat(TokenType::AssignmentOperator);

            auto valueExpr = table_or_list_or_cond_or();

            auto varNode =
                    new (MemoryLinearAllocate(&__ASTBuffer, sizeof(ASTVariable), alignof(ASTVariable)))
                            ASTVariable(t.text);
            auto node =
                    new (MemoryLinearAllocate(&__ASTBuffer, sizeof(ASTAssignListOrMapElement), alignof(ASTAssignListOrMapElement)))
                            ASTAssignListOrMapElement(varNode, indexExpr, valueExpr);
            return node;
        }
        else
        {
            auto t = currentToken;
            eat(TokenType::Identifier);
            eat(TokenType::AssignmentOperator);

            auto varNode =
                new (MemoryLinearAllocate(&__ASTBuffer, sizeof(ASTVariable), alignof(ASTVariable)))
                ASTVariable(t.text);

            auto node =
                new (MemoryLinearAllocate(&__ASTBuffer, sizeof(ASTAssignment), alignof(ASTAssignment)))
                ASTAssignment(varNode, table_or_list_or_cond_or());
            return node;
        }
    }
    else if (currentToken.type == TokenType::Return)
    {
        eat(TokenType::Return);
        auto node =
                new (MemoryLinearAllocate(&__ASTBuffer, sizeof(ASTReturn), alignof(ASTReturn)))
                        ASTReturn(cond_or());
        return node;
    }
    else if (currentToken.type == TokenType::If)
    {
        eat(TokenType::If);
        ASTNode* condition = cond_or();
        ASTNode* ifCase = statement_list();
        ASTNode* elseCase = nullptr;
        if (currentToken.type == TokenType::Else)
        {
            eat(TokenType::Else);
            elseCase = statement_list();
        }

        auto node =
                new (MemoryLinearAllocate(&__ASTBuffer, sizeof(ASTBranch), alignof(ASTBranch)))
                        ASTBranch(condition, ifCase, elseCase);
        return node;
    }
    else if (currentToken.type == TokenType::While)
    {
        eat(TokenType::While);
        ASTNode *condition = cond_or();
        ASTNode *body = statement_list();

        auto node = new (MemoryLinearAllocate(&__ASTBuffer, sizeof(ASTWhile), alignof(ASTWhile))) ASTWhile(condition, body);

        return node;
    }

    SendCompilationError("Unexpected token when expecting a new statement.", currentToken);
    return nullptr;
}

ASTNode* Parser::cond_expr()
{
    auto node = expr();

    if(ISANYOF4(currentToken.type, TokenType::LessThan, TokenType::LessThanOrEqual,
                TokenType::GreaterThan, TokenType::GreaterThanOrEqual))
    {
        auto t = currentToken;
        RelOp op = RelOp::LT;

        if (t.type == TokenType::LessThan)
        {
            eat(TokenType::LessThan);
            op = RelOp::LT;
        }
        else if (t.type == TokenType::LessThanOrEqual)
        {
            eat(TokenType::LessThanOrEqual);
            op = RelOp::LE;
        }
        else if (t.type == TokenType::GreaterThan)
        {
            eat(TokenType::GreaterThan);
            op = RelOp::GT;
        }
        else if (t.type == TokenType::GreaterThanOrEqual)
        {
            eat(TokenType::GreaterThanOrEqual);
            op = RelOp::GE;
        }

        node =
            new (MemoryLinearAllocate(&__ASTBuffer, sizeof(ASTRelOp), alignof(ASTRelOp)))
                    ASTRelOp(op, node, expr());
    }

    return node;
}

ASTNode* Parser::cond_equal()
{
    ASTNode* node = nullptr;

    if (currentToken.type == TokenType::LogicalNegation)
    {
        eat(TokenType::LogicalNegation);
        node = new (MemoryLinearAllocate(&__ASTBuffer, sizeof(ASTLogicalNot), alignof(ASTLogicalNot))) 
            ASTLogicalNot(cond_equal());
        return node;
    }

    node = cond_expr();

    if(ISANYOF2(currentToken.type, TokenType::Equal, TokenType::NotEqual))
    {
        auto t = currentToken;
        RelOp op = RelOp::EQ;

        if (t.type == TokenType::Equal)
        {
            eat(TokenType::Equal);
            op = RelOp::EQ;
        }
        else if (t.type == TokenType::NotEqual)
        {
            eat(TokenType::NotEqual);
            op = RelOp::NEQ;
        }

        node =
                new (MemoryLinearAllocate(&__ASTBuffer, sizeof(ASTRelOp), alignof(ASTRelOp)))
                        ASTRelOp(op, node, cond_expr());
    }

    return node;
}

ASTNode* Parser::cond_and()
{
    auto node = cond_equal();

    if(currentToken.type == TokenType::LogicalAnd)
    {
        eat(TokenType::LogicalAnd);

        node =
                new (MemoryLinearAllocate(&__ASTBuffer, sizeof(ASTRelOp), alignof(ASTRelOp)))
                        ASTRelOp(RelOp::AND, node, cond_equal());
    }

    return node;
}

ASTNode* Parser::cond_or()
{
    auto node = cond_and();

    if(currentToken.type == TokenType::LogicalOr)
    {
        eat(TokenType::LogicalOr);

        node =
                new (MemoryLinearAllocate(&__ASTBuffer, sizeof(ASTRelOp), alignof(ASTRelOp)))
                        ASTRelOp(RelOp::OR, node, cond_and());
    }

    return node;
}

ASTNode* Parser::factor()
{
    auto t = currentToken;
    ASTNode* node = nullptr;

    if (t.type == TokenType::LParen)
    {
        eat(TokenType::LParen);
        node = cond_or(); // at which point if the smallest factor starts with lparen then we go all the way back to cond_or()! That's right!
        eat(TokenType::RParen);
    }
    else if (t.type == TokenType::NumberLiteral)
    {
        eat(TokenType::NumberLiteral);
        if (t.text.find('.') == std::string::npos)
        {
            node = new(MemoryLinearAllocate(&__ASTBuffer, sizeof(ASTNumberTerminal), alignof(ASTNumberTerminal)))
                ASTNumberTerminal(std::stod(t.text), true);
        }
        else
        {
            node = new(MemoryLinearAllocate(&__ASTBuffer, sizeof(ASTNumberTerminal), alignof(ASTNumberTerminal)))
                ASTNumberTerminal(std::stod(t.text), false);
        }
        return node;
    }
    else if (t.type == TokenType::StringLiteral)
    {
        eat(TokenType::StringLiteral);
        node = new(MemoryLinearAllocate(&__ASTBuffer, sizeof(ASTStringTerminal), alignof(ASTStringTerminal)))
            ASTStringTerminal(t.text);
    }
    else if (t.type == TokenType::True)
    {
        eat(TokenType::True);
        node = new(MemoryLinearAllocate(&__ASTBuffer, sizeof(ASTBooleanTerminal), alignof(ASTBooleanTerminal)))
            ASTBooleanTerminal(true);
    }
    else if (t.type == TokenType::False)
    {
        eat(TokenType::False);
        node = new(MemoryLinearAllocate(&__ASTBuffer, sizeof(ASTBooleanTerminal), alignof(ASTBooleanTerminal)))
            ASTBooleanTerminal(false);
    }
    else if (t.type == TokenType::Identifier)
    {
        auto nextToken = tokens.at(currentTokenIndex + 1);

        if (nextToken.type == TokenType::LParen)
        {
            node = procedure_call();
        }
        else if (nextToken.type == TokenType::LSqBrack)
        {
            auto t = currentToken;

            eat(TokenType::Identifier);
            eat(TokenType::LSqBrack);

            auto indexExpr = expr();

            eat(TokenType::RSqBrack);

            auto varNode =
                new (MemoryLinearAllocate(&__ASTBuffer, sizeof(ASTVariable), alignof(ASTVariable)))
                ASTVariable(t.text);
            node =
                new (MemoryLinearAllocate(&__ASTBuffer, sizeof(ASTAccessListOrMapElement), alignof(ASTAccessListOrMapElement)))
                ASTAccessListOrMapElement(varNode, indexExpr);
        }
        else
        {
            eat(TokenType::Identifier);
            node =
                new (MemoryLinearAllocate(&__ASTBuffer, sizeof(ASTVariable), alignof(ASTVariable)))
                        ASTVariable(t.text);
        }
    }
    else
    {
        SendCompilationError("Unknown token.", t);
    }

    return node;
}

ASTNode* Parser::term()
{
    auto node = factor();

    while(ISANYOF2(currentToken.type, TokenType::MulOperator, TokenType::DivOperator))
    {
        auto t = currentToken;
        BinOp op = BinOp::Mul;

        if (t.type == TokenType::MulOperator)
        {
            eat(TokenType::MulOperator);
            op = BinOp::Mul;
        }
        else if (t.type == TokenType::DivOperator)
        {
            eat(TokenType::DivOperator);
            op = BinOp::Div;
        }

        node =
            new(MemoryLinearAllocate(&__ASTBuffer, sizeof(ASTBinOp), alignof(ASTBinOp)))
                    ASTBinOp(op, node, factor());
    }

    return node;
}

ASTNode* Parser::expr()
{
    auto node = term();

    while(ISANYOF2(currentToken.type, TokenType::AddOperator, TokenType::SubOperator))
    {
        auto t = currentToken;
        BinOp op = BinOp::Add;

        if (t.type == TokenType::AddOperator)
        {
            eat(TokenType::AddOperator);
            op = BinOp::Add;
        }
        else if (t.type == TokenType::SubOperator)
        {
            eat(TokenType::SubOperator);
            op = BinOp::Sub;
        }

        node =
            new(MemoryLinearAllocate(&__ASTBuffer, sizeof(ASTBinOp), alignof(ASTBinOp)))
                    ASTBinOp(op, node, term());
    }

    return node;
}

void Parser::eat(TokenType tpe)
{
    // compare the current token type with the passed token
    // type and if they match then "eat" the current token
    // and assign the next token to currentToken, otherwise
    // error.

    if(currentToken.type == tpe)
    {
        currentToken = tokens[++currentTokenIndex];
    }
    else
    {
        SendCompilationError("Ate unexpected token.", currentToken);
    }
}

#pragma endregion ASTAndParser

#pragma region Interpreter

static void
InterpretStatementList(ASTNode* statements);

static TValue
InterpretProcedureCall(ASTProcedureCall* procedureCall);

static TValue
InterpretExpression(ASTNode* ast)
{
    // PLProfilerPush(PLPROFILER_INTERPRETER_SYSTEM::INTERPRET_EXPRESSION);

    TValue result;

    switch(ast->GetType())
    {
        case ASTNodeType::SIMPLYTVALUE: 
        {
            // PLProfilerBegin(PLPROFILER_BINOP_RELOP);
            auto v = static_cast<ASTSimplyTValue*>(ast);
            result = v->value;
            // PLProfilerEnd(PLPROFILER_BINOP_RELOP);
            break;
        }
        case ASTNodeType::BINOP:
        {
            auto v = static_cast<ASTBinOp*>(ast);
            // PLProfilerBegin(PLPROFILER_BINOP_RELOP);
            auto ln = v->left;
            auto rn = v->right;
            // PLProfilerEnd(PLPROFILER_BINOP_RELOP);
            TValue l = InterpretExpression(ln);
            TValue r = InterpretExpression(rn);
            // both integer, then integer
            // both float, then float
            // one int, one float, then float
            // PLProfilerBegin(PLPROFILER_BINOP_RELOP);
            TValue::ValueType retValType = TValue::ValueType::Integer;
            switch (v->op)
            {
                case BinOp::Add:
                {
                    if (l.type == TValue::ValueType::Integer && r.type == TValue::ValueType::Integer)
                    {
                        result.integerValue = l.integerValue + r.integerValue;
                        result.type = TValue::ValueType::Integer;
                    }
                    else if (l.type == TValue::ValueType::Real && r.type == TValue::ValueType::Real)
                    {
                        result.realValue = l.realValue + r.realValue;
                        result.type = TValue::ValueType::Real;
                    }
                    else if (l.type == TValue::ValueType::Integer && r.type == TValue::ValueType::Real)
                    {
                        result.realValue = l.integerValue + r.realValue;
                        result.type = TValue::ValueType::Real;
                    }
                    else if (l.type == TValue::ValueType::Real && r.type == TValue::ValueType::Integer)
                    {
                        result.realValue = l.realValue + r.integerValue;
                        result.type = TValue::ValueType::Real;
                    }
                    else
                    {
                        // todo error
                    }
                    break;
                }
                case BinOp::Sub:
                {
                    if (l.type == TValue::ValueType::Integer && r.type == TValue::ValueType::Integer)
                    {
                        result.integerValue = l.integerValue - r.integerValue;
                        result.type = TValue::ValueType::Integer;
                    }
                    else if (l.type == TValue::ValueType::Real && r.type == TValue::ValueType::Real)
                    {
                        result.realValue = l.realValue - r.realValue;
                        result.type = TValue::ValueType::Real;
                    }
                    else if (l.type == TValue::ValueType::Integer && r.type == TValue::ValueType::Real)
                    {
                        result.realValue = l.integerValue - r.realValue;
                        result.type = TValue::ValueType::Real;
                    }
                    else if (l.type == TValue::ValueType::Real && r.type == TValue::ValueType::Integer)
                    {
                        result.realValue = l.realValue - r.integerValue;
                        result.type = TValue::ValueType::Real;
                    }
                    else
                    {
                        // todo error
                    }
                    break;
                }
                case BinOp::Mul:
                {
                    if (l.type == TValue::ValueType::Integer && r.type == TValue::ValueType::Integer)
                    {
                        result.integerValue = l.integerValue * r.integerValue;
                        result.type = TValue::ValueType::Integer;
                    }
                    else if (l.type == TValue::ValueType::Real && r.type == TValue::ValueType::Real)
                    {
                        result.realValue = l.realValue * r.realValue;
                        result.type = TValue::ValueType::Real;
                    }
                    else if (l.type == TValue::ValueType::Integer && r.type == TValue::ValueType::Real)
                    {
                        result.realValue = l.integerValue * r.realValue;
                        result.type = TValue::ValueType::Real;
                    }
                    else if (l.type == TValue::ValueType::Real && r.type == TValue::ValueType::Integer)
                    {
                        result.realValue = l.realValue * r.integerValue;
                        result.type = TValue::ValueType::Real;
                    }
                    else
                    {
                        // todo error
                    }
                    break;
                }
                case BinOp::Div:
                {
                    if (l.type == TValue::ValueType::Integer && r.type == TValue::ValueType::Integer)
                    {
                        result.integerValue = l.integerValue / r.integerValue;
                        result.type = TValue::ValueType::Integer;
                    }
                    else if (l.type == TValue::ValueType::Real && r.type == TValue::ValueType::Real)
                    {
                        result.realValue = l.realValue / r.realValue;
                        result.type = TValue::ValueType::Real;
                    }
                    else if (l.type == TValue::ValueType::Integer && r.type == TValue::ValueType::Real)
                    {
                        result.realValue = l.integerValue / r.realValue;
                        result.type = TValue::ValueType::Real;
                    }
                    else if (l.type == TValue::ValueType::Real && r.type == TValue::ValueType::Integer)
                    {
                        result.realValue = l.realValue / r.integerValue;
                        result.type = TValue::ValueType::Real;
                    }
                    else
                    {
                        // todo error
                    }
                    break;
                }
                default:
                {
                    SendRuntimeException("Unknown BINOP expression.");
                    break;
                }
            }
            // PLProfilerEnd(PLPROFILER_BINOP_RELOP);
            break;
        }
        
        case ASTNodeType::RELOP:
        {
            // PLProfilerBegin(PLPROFILER_BINOP_RELOP);
            auto v = static_cast<ASTRelOp*>(ast);
            auto ln = v->left;
            auto rn = v->right;
            // PLProfilerEnd(PLPROFILER_BINOP_RELOP);
            TValue l = InterpretExpression(ln);
            TValue r = InterpretExpression(rn);
            // PLProfilerBegin(PLPROFILER_BINOP_RELOP);
            if (l.type == TValue::ValueType::Integer)
            {
                l.realValue = double(l.integerValue);
                l.type = TValue::ValueType::Real;
            }
            else if (l.type == TValue::ValueType::Boolean)
            {
                l.realValue = l.boolValue ? 1.0 : 0.0;
                l.type = TValue::ValueType::Real;
            }
            if (r.type == TValue::ValueType::Integer)
            {
                r.realValue = double(r.integerValue);
                r.type = TValue::ValueType::Real;
            }
            else if (r.type == TValue::ValueType::Boolean)
            {
                r.realValue = r.boolValue ? 1.0 : 0.0;
                r.type = TValue::ValueType::Real;
            }
            switch (v->op)
            {
                case RelOp::LT:
                    result.boolValue = l.realValue < r.realValue;
                    result.type = TValue::ValueType::Boolean;
                    break; 
                case RelOp::LE:
                    result.boolValue = l.realValue <= r.realValue;
                    result.type = TValue::ValueType::Boolean;
                    break;
                case RelOp::GT:
                    result.boolValue = l.realValue > r.realValue;
                    result.type = TValue::ValueType::Boolean;
                    break;
                case RelOp::GE:
                    result.boolValue = l.realValue >= r.realValue;
                    result.type = TValue::ValueType::Boolean;
                    break;
                case RelOp::EQ:
                    result.boolValue = l.realValue == r.realValue;
                    result.type = TValue::ValueType::Boolean;
                    break;
                case RelOp::NEQ:
                    result.boolValue = l.realValue != r.realValue;
                    result.type = TValue::ValueType::Boolean;
                    break;
                case RelOp::AND:
                    result.boolValue = (l.realValue != 0.0 && r.realValue != 0.0);
                    result.type = TValue::ValueType::Boolean;
                    break;
                case RelOp::OR:
                    result.boolValue = (l.realValue != 0.0 || r.realValue != 0.0);
                    result.type = TValue::ValueType::Boolean;
                    break;
                default:
                    SendRuntimeException("Unknown RELOP expression.");
                    break;
            }
            // PLProfilerEnd(PLPROFILER_BINOP_RELOP);
            break;
        }
        case ASTNodeType::LOGICALNOT:
        {
            auto v = static_cast<ASTLogicalNot*>(ast);
            result = InterpretExpression(v->boolExpr);
            result.boolValue = !result.boolValue;
            break;
        }

        case ASTNodeType::VARIABLE:
        {
            // PLProfilerBegin(PLPROFILER_BINOP_RELOP);
            auto v = static_cast<ASTVariable*>(ast);
            if (__MSRuntime.activeEnv.KeyExistsInAccessibleScopes(v->id))
            {
                result = __MSRuntime.activeEnv.AccessAtKeyInAccessibleScopes(v->id);
            }
            else if (__MSRuntime.globalEnv.Contains(v->id))
            {
                result = __MSRuntime.globalEnv.AccessMapEntry(v->id);
            }
            else
            {
                SendRuntimeException("Unknown identifier in expression.");
            }
            // PLProfilerEnd(PLPROFILER_BINOP_RELOP);
            break;
        }

        case ASTNodeType::ACCESS_LIST_OR_MAP_ELEMENT:
        {
            auto v = static_cast<ASTAccessListOrMapElement*>(ast);

            auto indexTValue = InterpretExpression(v->indexExpression);

            // PLProfilerBegin(PLPROFILER_BINOP_RELOP);
            ASSERT(indexTValue.type == TValue::ValueType::Integer || indexTValue.type == TValue::ValueType::GCObject);

            ASSERT(v->listOrMapVariableName->GetType() == ASTNodeType::VARIABLE);
            std::string listOrMapVariableKey = static_cast<ASTVariable*>(v->listOrMapVariableName)->id;

            i64 gcObjectId = 0;
            if (__MSRuntime.activeEnv.KeyExistsInAccessibleScopes(listOrMapVariableKey))
            {
                TValue listOrMapGCObj = __MSRuntime.activeEnv.AccessAtKeyInAccessibleScopes(listOrMapVariableKey);
                ASSERT(listOrMapGCObj.type == TValue::ValueType::GCObject);
                gcObjectId = listOrMapGCObj.GCReferenceObject;
            }
            else if (__MSRuntime.globalEnv.Contains(listOrMapVariableKey))
            {
                TValue listOrMapGCObj = __MSRuntime.globalEnv.AccessMapEntry(listOrMapVariableKey);
                ASSERT(listOrMapGCObj.type == TValue::ValueType::GCObject);
                gcObjectId = listOrMapGCObj.GCReferenceObject;
            }
            else
            {
                // error
                ASSERT(0);
            }

            MesaGCObject::GCObjectType dataStructureType = GetTypeOfGCObject(gcObjectId);
            ASSERT(dataStructureType == MesaGCObject::GCObjectType::List || dataStructureType == MesaGCObject::GCObjectType::Table);
            if (dataStructureType == MesaGCObject::GCObjectType::List)
            {
                ASSERT(indexTValue.type == TValue::ValueType::Integer);
                const i64 listIndex = indexTValue.integerValue;
                // todo assert integer is non negative, valid, etc.
                MesaScript_List* list = AccessMesaScriptList(gcObjectId);
                result = list->AccessListEntry(listIndex);
            }
            else
            {
                ASSERT(indexTValue.type == TValue::ValueType::GCObject);
                ASSERT(GetTypeOfGCObject(indexTValue.GCReferenceObject) == MesaGCObject::GCObjectType::String);
                const std::string& tableKey = AccessMesaScriptString(indexTValue.GCReferenceObject)->text;

                MesaScript_Table* table = AccessMesaScriptTable(gcObjectId);
                result = table->AccessMapEntry(tableKey);
            }

            // PLProfilerEnd(PLPROFILER_BINOP_RELOP);
            break;
        }

        case ASTNodeType::CREATETABLE:
        {
            //auto v = static_cast<CreateNewMapEntry*>(ast);
            // PLProfilerBegin(PLPROFILER_BINOP_RELOP);
            result.type = TValue::ValueType::GCObject;
            result.GCReferenceObject = RequestNewGCObject(MesaGCObject::GCObjectType::Table);
            // PLProfilerEnd(PLPROFILER_BINOP_RELOP);
            break;
        }

        case ASTNodeType::CREATELIST:
        {
            // PLProfilerBegin(PLPROFILER_BINOP_RELOP);
            auto v = static_cast<ASTCreateList*>(ast);
            result.type = TValue::ValueType::GCObject;
            result.GCReferenceObject = RequestNewGCObject(MesaGCObject::GCObjectType::List);
            MesaScript_List* mesaList = AccessMesaScriptList(result.GCReferenceObject);
            __MSRuntime.activeEnv.InsertTransientObject(result.GCReferenceObject, 0);
            // PLProfilerEnd(PLPROFILER_BINOP_RELOP);
            for (int i = 0; i < v->listInitializingElements.size(); ++i)
            {
                ASTNode* elementExpr = v->listInitializingElements[i];
                TValue elementValue = InterpretExpression(elementExpr);
                // Return value gets captured here too
                mesaList->Append(elementValue);
            }
            break;
        }

        case ASTNodeType::NUMBER:
        {
            // PLProfilerBegin(PLPROFILER_BINOP_RELOP);
            auto v = static_cast<ASTNumberTerminal*>(ast);
            if (v->isInteger)
            {
                result.integerValue = i64(v->value);
                result.type = TValue::ValueType::Integer;
            }
            else
            {
                result.realValue = v->value;
                result.type = TValue::ValueType::Real;
            }
            // PLProfilerEnd(PLPROFILER_BINOP_RELOP);
            break;
        }
        case ASTNodeType::STRING:
        {
            // PLProfilerBegin(PLPROFILER_BINOP_RELOP);
            auto v = static_cast<ASTStringTerminal*>(ast);
            result.type = TValue::ValueType::GCObject;
            result.GCReferenceObject = RequestNewGCObject(MesaGCObject::GCObjectType::String);
            MesaScript_String *createdString = AccessMesaScriptString(result.GCReferenceObject);
            createdString->text = v->value;
            __MSRuntime.activeEnv.InsertTransientObject(result.GCReferenceObject, 0);
            // PLProfilerEnd(PLPROFILER_BINOP_RELOP);
            break;
        }
        case ASTNodeType::BOOLEAN:
        {
            // PLProfilerBegin(PLPROFILER_BINOP_RELOP);
            auto v = static_cast<ASTBooleanTerminal*>(ast);
            result.boolValue = v->value;
            result.type = TValue::ValueType::Boolean;
            // PLProfilerEnd(PLPROFILER_BINOP_RELOP);
            break;
        }
        case ASTNodeType::PROCEDURECALL:
        {
            auto v = static_cast<ASTProcedureCall*>(ast);
            result = InterpretProcedureCall(v);
            break;
        }
    }

    // PLProfilerBegin(PLPROFILER_BINOP_RELOP);
    if (result.type == TValue::ValueType::Invalid)
    {
        SendRuntimeException("Invalid expression.");
    }
    else if (result.type == TValue::ValueType::GCObject
        && GetTypeOfGCObject(result.GCReferenceObject) == MesaGCObject::GCObjectType::String
        && !__MSRuntime.activeEnv.TransientObjectExists(result.GCReferenceObject))
    {
        i64 stringCopyId = RequestNewGCObject(MesaGCObject::GCObjectType::String);
        AccessMesaScriptString(stringCopyId)->text = AccessMesaScriptString(result.GCReferenceObject)->text;
        result.GCReferenceObject = stringCopyId;
        __MSRuntime.activeEnv.InsertTransientObject(stringCopyId, 0);
    }
    // PLProfilerEnd(PLPROFILER_BINOP_RELOP);

    // PLProfilerPop();

    return result;
}

static void
InterpretStatement(ASTNode* statement)
{
    // PLProfilerPush(PLPROFILER_INTERPRETER_SYSTEM::INTERPRET_STATEMENT);

    switch (statement->GetType())
    {
        case ASTNodeType::PROCEDURECALL: {
            auto v = static_cast<ASTProcedureCall*>(statement);
            InterpretProcedureCall(v);
            // This is the only place return value is wasted
        } break;

        case ASTNodeType::ASSIGN: {
            auto v = static_cast<ASTAssignment*>(statement);
            
            // Return value gets captured here too
            auto result = InterpretExpression(v->expr);

            ASSERT(v->id->GetType() == ASTNodeType::VARIABLE);
            std::string key = static_cast<ASTVariable*>(v->id)->id;
            if (__MSRuntime.activeEnv.KeyExistsInAccessibleScopes(key))
            {
                __MSRuntime.activeEnv.ReplaceAtKeyInAccessibleScopes(key, result);
            }
            else if (__MSRuntime.globalEnv.Contains(key))
            {
                __MSRuntime.globalEnv.ReplaceMapEntryAtKey(key, result);
            }
            else
            {
                __MSRuntime.activeEnv.EmplaceNewElement(key, result);
            }
        } break;
        case ASTNodeType::ASSIGN_LIST_OR_MAP_ELEMENT: {
            auto v = static_cast<ASTAssignListOrMapElement*>(statement);

            auto indexTValue = InterpretExpression(v->indexExpression);
            ASSERT(indexTValue.type == TValue::ValueType::Integer || indexTValue.type == TValue::ValueType::GCObject);

            ASSERT(v->listOrMapVariableName->GetType() == ASTNodeType::VARIABLE);
            std::string listOrMapVariableKey = static_cast<ASTVariable*>(v->listOrMapVariableName)->id;

            i64 gcObjectId = 0;
            if (__MSRuntime.activeEnv.KeyExistsInAccessibleScopes(listOrMapVariableKey))
            {
                TValue listOrMapGCObj = __MSRuntime.activeEnv.AccessAtKeyInAccessibleScopes(listOrMapVariableKey);
                ASSERT(listOrMapGCObj.type == TValue::ValueType::GCObject);
                gcObjectId = listOrMapGCObj.GCReferenceObject;
            }
            else if (__MSRuntime.globalEnv.Contains(listOrMapVariableKey))
            {
                TValue listOrMapGCObj = __MSRuntime.globalEnv.AccessMapEntry(listOrMapVariableKey);
                ASSERT(listOrMapGCObj.type == TValue::ValueType::GCObject);
                gcObjectId = listOrMapGCObj.GCReferenceObject;
            }
            else
            {
                SendRuntimeException("Cannot find a list or map with the given identifier.");
            }

            // Return value gets captured here too
            auto valueTValue = InterpretExpression(v->valueExpression);

            MesaGCObject::GCObjectType dataStructureType = GetTypeOfGCObject(gcObjectId);
            ASSERT(dataStructureType == MesaGCObject::GCObjectType::List || dataStructureType == MesaGCObject::GCObjectType::Table);
            if(dataStructureType == MesaGCObject::GCObjectType::List)
            {
                ASSERT(indexTValue.type == TValue::ValueType::Integer);
                const i64 listIndex = indexTValue.integerValue;
                // todo assert integer is non negative, valid, etc.
                
                MesaScript_List* listToModify = AccessMesaScriptList(gcObjectId);
                // todo assert index exists in list
                listToModify->ReplaceListEntryAtIndex(listIndex, valueTValue);
            }
            else
            {
                ASSERT(indexTValue.type == TValue::ValueType::GCObject);
                ASSERT(GetTypeOfGCObject(indexTValue.GCReferenceObject) == MesaGCObject::GCObjectType::String);
                const std::string& tableKey = AccessMesaScriptString(indexTValue.GCReferenceObject)->text;

                MesaScript_Table* tableToModify = AccessMesaScriptTable(gcObjectId);
                if(tableToModify->Contains(tableKey))
                {
                    tableToModify->ReplaceMapEntryAtKey(tableKey, valueTValue);
                }
                else
                {
                    tableToModify->CreateNewMapEntry(tableKey, valueTValue);
                }
            }

        } break;

        case ASTNodeType::RETURN: {
            auto v = static_cast<ASTReturn*>(statement);
            TValue result = InterpretExpression(v->expr);
            returnValue = result;
            returnValueSetFlag = true;
            returnRequestedFlag = true;
            // 2023-12-24: Transient GCOBJ does not get captured when we pass it along via RETURN
            // so I must move the transient id a level above. Moving the InsertTransientObj call here.
            if (result.type == TValue::ValueType::GCObject)
            {
                if (__MSRuntime.activeEnv.TransientObjectExistsInActiveScope(result.GCReferenceObject))
                    __MSRuntime.activeEnv.EraseTransientObject(result.GCReferenceObject);
                __MSRuntime.activeEnv.InsertTransientObject(result.GCReferenceObject, -1);
            }
        } break;

        case ASTNodeType::BRANCH: {
            auto v = static_cast<ASTBranch*>(statement);
            auto condition = InterpretExpression(v->condition);
            ASSERT(condition.type == TValue::ValueType::Boolean);
            if (condition.boolValue)
                InterpretStatementList(v->if_body);
            else if (v->else_body)
                InterpretStatementList(v->else_body);
        } break;

        case ASTNodeType::WHILE: {
            auto v = static_cast<ASTWhile*>(statement);

            auto condition = InterpretExpression(v->condition);
            ASSERT(condition.type == TValue::ValueType::Boolean);
            for (; condition.boolValue; condition = InterpretExpression(v->condition))
            {
                InterpretStatementList(v->body);
            }
        } break;
    }

    // ALL STATEMENTS MUST REACH THIS POINT
    __MSRuntime.activeEnv.ClearTransientsInLastScope();

    // PLProfilerPop();
}

static void
InterpretStatementList(ASTNode* statements)
{
    ASSERT(statements->GetType() == ASTNodeType::STATEMENTLIST);
    auto v = static_cast<ASTStatementList*>(statements);
    for (const auto& s : v->statements)
    {
        InterpretStatement(s);
        if (returnRequestedFlag) return;
    }
}


typedef TValue (*cpp_bound_fn_no_param_t)();
typedef TValue (*cpp_bound_fn_one_param_t)(TValue);
typedef TValue (*cpp_bound_fn_two_param_t)(TValue, TValue);
typedef TValue (*cpp_bound_fn_three_param_t)(TValue, TValue, TValue);
typedef TValue (*cpp_bound_fn_four_param_t)(TValue, TValue, TValue, TValue);
typedef TValue (*cpp_bound_fn_five_param_t)(TValue, TValue, TValue, TValue, TValue);
typedef TValue (*cpp_bound_fn_six_param_t)(TValue, TValue, TValue, TValue, TValue, TValue);

struct cpp_bound_fn_data
{
    int argc = 0;
    cpp_bound_fn_no_param_t f0 = NULL;
    cpp_bound_fn_one_param_t f1 = NULL;
    cpp_bound_fn_two_param_t f2 = NULL;
    cpp_bound_fn_three_param_t f3 = NULL;
    cpp_bound_fn_four_param_t f4 = NULL;
    cpp_bound_fn_five_param_t f5 = NULL;
    cpp_bound_fn_six_param_t f6 = NULL;
};

static std::unordered_map<std::string, cpp_bound_fn_data> cpp_fn_registry;

void pipl_bind_cpp_fn(const char *fn_name, int argc, void *fn_ptr)
{
    cpp_bound_fn_data fn_data;
    fn_data.argc = argc;
    switch(argc)
    {
    case 0: fn_data.f0 = (cpp_bound_fn_no_param_t)fn_ptr; break;
    case 1: fn_data.f1 = (cpp_bound_fn_one_param_t)fn_ptr; break;
    case 2: fn_data.f2 = (cpp_bound_fn_two_param_t)fn_ptr; break;
    case 3: fn_data.f3 = (cpp_bound_fn_three_param_t)fn_ptr; break;
    case 4: fn_data.f4 = (cpp_bound_fn_four_param_t)fn_ptr; break;
    case 5: fn_data.f5 = (cpp_bound_fn_five_param_t)fn_ptr; break;
    case 6: fn_data.f6 = (cpp_bound_fn_six_param_t)fn_ptr; break;
    default: ASSERT(false);
    }
    cpp_fn_registry.insert({ fn_name, fn_data });
}

static TValue
InterpretProcedureCall(ASTProcedureCall *procedureCall) // NEVER CALL UNLESS PART OF INTERPRETER, USE INTERPRETSTATEMENT INSTEAD
{
    // PLProfilerPush(PLPROFILER_INTERPRETER_SYSTEM::INTERPRET_PROCEDURE_CALL);

    TValue procedureVariable;
    if (__MSRuntime.activeEnv.KeyExistsTopLevel(procedureCall->id))
    {
        procedureVariable = __MSRuntime.activeEnv.AccessAtKeyInAccessibleScopes(procedureCall->id);
    }
    else if (__MSRuntime.globalEnv.Contains(procedureCall->id))
    {
        procedureVariable = __MSRuntime.globalEnv.AccessMapEntry(procedureCall->id);
    }
    else // only reach this case if valid CPPBOUND function or unknown function identifier error
    {
        auto fn_data_iter = cpp_fn_registry.find(procedureCall->id);
        if (fn_data_iter != cpp_fn_registry.end())
        {
            auto fn_data = fn_data_iter->second;
            ASSERT(procedureCall->argsExpressions.size() == fn_data.argc);
            switch (fn_data.argc)
            {
            case 0: 
            {
                return fn_data.f0();
            }
            case 1: 
            {
                TValue arg0 = InterpretExpression(procedureCall->argsExpressions[0]);
                return fn_data.f1(arg0);
            }
            case 2: 
            {
                TValue arg0 = InterpretExpression(procedureCall->argsExpressions[0]);
                TValue arg1 = InterpretExpression(procedureCall->argsExpressions[1]);
                return fn_data.f2(arg0, arg1);
            }
            case 3: 
            {
                TValue arg0 = InterpretExpression(procedureCall->argsExpressions[0]);
                TValue arg1 = InterpretExpression(procedureCall->argsExpressions[1]);
                TValue arg2 = InterpretExpression(procedureCall->argsExpressions[2]);
                return fn_data.f3(arg0, arg1, arg2);
            }
            case 4: 
            {
                TValue arg0 = InterpretExpression(procedureCall->argsExpressions[0]);
                TValue arg1 = InterpretExpression(procedureCall->argsExpressions[1]);
                TValue arg2 = InterpretExpression(procedureCall->argsExpressions[2]);
                TValue arg3 = InterpretExpression(procedureCall->argsExpressions[3]);
                return fn_data.f4(arg0, arg1, arg2, arg3);
            }
            case 5: 
            {
                TValue arg0 = InterpretExpression(procedureCall->argsExpressions[0]);
                TValue arg1 = InterpretExpression(procedureCall->argsExpressions[1]);
                TValue arg2 = InterpretExpression(procedureCall->argsExpressions[2]);
                TValue arg3 = InterpretExpression(procedureCall->argsExpressions[3]);
                TValue arg4 = InterpretExpression(procedureCall->argsExpressions[4]);
                return fn_data.f5(arg0, arg1, arg2, arg3, arg4);
            }
            case 6:
            {
                TValue arg0 = InterpretExpression(procedureCall->argsExpressions[0]);
                TValue arg1 = InterpretExpression(procedureCall->argsExpressions[1]);
                TValue arg2 = InterpretExpression(procedureCall->argsExpressions[2]);
                TValue arg3 = InterpretExpression(procedureCall->argsExpressions[3]);
                TValue arg4 = InterpretExpression(procedureCall->argsExpressions[4]);
                TValue arg5 = InterpretExpression(procedureCall->argsExpressions[5]);
                return fn_data.f6(arg0, arg1, arg2, arg3, arg4, arg5);
            }
            default: ASSERT(false);
            }
        }
        else
        {
            SendRuntimeException("Unknown identifier for procedure call.");
        }
    }
    auto procedureDefinition = PROCEDURES_DATABASE.At((unsigned int)procedureVariable.procedureId);


    // Intended: creating function scope (map) and adding the function arguments as map entries increments their ref count
    MesaScript_Table functionScope;
    for (int i = 0; i < procedureDefinition.args.size(); ++i)
    {
        auto argname = procedureDefinition.args[i];
        auto argexpr = procedureCall->argsExpressions[i];
        TValue argv = InterpretExpression(argexpr);
        functionScope.CreateNewMapEntry(argname, argv);
    }
    __MSRuntime.activeEnv.PushScope(&functionScope);

    TValue retval;
    returnRequestedFlag = false;
    returnValueSetFlag = false;

    InterpretStatementList(procedureDefinition.body);

    if (returnValueSetFlag)
        retval = returnValue;

    returnRequestedFlag = false;
    returnValueSetFlag = false;

    // Intended: activeEnv.PopScope should decrement ref count of every entry of the function scope being popped.
    __MSRuntime.activeEnv.PopScope();

    // PLProfilerPop();

    return retval;
}

#pragma endregion Interpreter

void CompileAndRunMesaScriptCode(const std::string& script, MesaScript_Table *scriptScope)
{
    __MSRuntime.activeEnv.PushScope(scriptScope);
    std::vector<Token> tokens = Lexer(script.c_str());
    auto parser = Parser(tokens);
    parser.parse();

    // TODO(Kevin): if not release mode, probably want to keep Parser paired with the scriptScope so we have debug data later

    for (auto& scriptTopLevelStatement : parser.scriptExecutionQueue)
    {
        InterpretStatement(scriptTopLevelStatement);
    }

    __MSRuntime.activeEnv.PopScope();
}

void SetEnvironmentScope(MesaScript_Table *scriptScope)
{
    ClearEnvironmentScope();
    __MSRuntime.activeEnv.PushScope(scriptScope);
}

void ClearEnvironmentScope()
{
    if (__MSRuntime.activeEnv.ScopesSize() > 0)
    {
        ASSERT(__MSRuntime.activeEnv.ScopesSize() == 1);
        __MSRuntime.activeEnv.PopScope();
    }
}

void CallFunction_Parameterless(const char* functionIdentifier)
{
    ASTProcedureCall parameterlessProcCallNode = ASTProcedureCall(functionIdentifier);
    InterpretStatement(&parameterlessProcCallNode);
}

void CallFunction_OneParam(const char* functionIdentifier, TValue arg0)
{
    ASTProcedureCall procCallNode = ASTProcedureCall(functionIdentifier);
    ASTSimplyTValue arg0Node = ASTSimplyTValue(arg0);
    procCallNode.argsExpressions.push_back(&arg0Node);
    InterpretStatement(&procCallNode);
}


TValue CPPBOUND_MESASCRIPT_Print(TValue value)
{
    PLProfilerBegin(PLPROFILER_PRINT);
    if (value.type == TValue::ValueType::Integer)
    {
        printf("%lld\n", value.integerValue);
    }
    else if (value.type == TValue::ValueType::Boolean)
    {
        printf("%s\n", (value.boolValue ? "true" : "false"));
    }
    else if (value.type == TValue::ValueType::Real)
    {
        printf("%lf\n", value.realValue);
    }
    else if (value.type == TValue::ValueType::GCObject)
    {
        if (GetTypeOfGCObject(value.GCReferenceObject) == MesaGCObject::GCObjectType::Table)
        {
            printf("printing table\n");
            for (const auto& pair : AccessMesaScriptTable(value.GCReferenceObject)->table)
            {
                if (pair.second.type == TValue::ValueType::Integer)
                {
                    printf("\t%s : %lld\n", pair.first.c_str(), pair.second.integerValue);
                }
                else if (pair.second.type == TValue::ValueType::Boolean)
                {
                    printf("\t%s : %s\n", pair.first.c_str(), (pair.second.boolValue ? "true" : "false"));
                }
                else if (pair.second.type == TValue::ValueType::Real)
                {
                    printf("\t%s : %lf\n", pair.first.c_str(), pair.second.realValue);
                }
                else if (pair.second.type == TValue::ValueType::GCObject)
                {
                    printf("\t%s : gcobject\n", pair.first.c_str());
                }
            }
        }
        else if (GetTypeOfGCObject(value.GCReferenceObject) == MesaGCObject::GCObjectType::List)
        {
            printf("printing list\n");
            for (const auto& element : AccessMesaScriptList(value.GCReferenceObject)->list)
            {
                if (element.type == TValue::ValueType::Integer)
                {
                    printf("\t%lld\n", element.integerValue);
                }
                else if (element.type == TValue::ValueType::Boolean)
                {
                    printf("\t%s\n", (element.boolValue ? "true" : "false"));
                }
                else if (element.type == TValue::ValueType::Real)
                {
                    printf("\t%lf\n", element.realValue);
                }
                else if (element.type == TValue::ValueType::GCObject)
                {
                    printf("\tgcobject\n");
                }
            }  
        }
        else if (GetTypeOfGCObject(value.GCReferenceObject) == MesaGCObject::GCObjectType::String)
        {
            printf("%s\n", AccessMesaScriptString(value.GCReferenceObject)->text.c_str());
        }
    }

    PLProfilerEnd(PLPROFILER_PRINT);
    return TValue();
}

TValue CPPBOUND_MESASCRIPT_RefCount(TValue gcobj)
{
    ASSERT(gcobj.type == TValue::ValueType::GCObject);

    TValue result;
    result.type = TValue::ValueType::Integer;
    result.integerValue = (int)GCOBJECTS_DATABASE.at(gcobj.GCReferenceObject)->refCount;
    return result;
}

void InitializeLanguageCompilerAndRuntime()
{
    MemoryLinearInitialize(&__ASTBuffer, 8000000);

    pipl_bind_cpp_fn("print", 1, CPPBOUND_MESASCRIPT_Print);
    pipl_bind_cpp_fn("refcount", 1, CPPBOUND_MESASCRIPT_RefCount);

    std::string mesaScriptSetupCode = "fn add(x, y) { return x + y } fn checkeq(expected, actual) { if (expected == actual) { print('test pass') } else { print('test fail') } }";
    CompileAndRunMesaScriptCode(mesaScriptSetupCode, &__MSRuntime.globalEnv); // hack to add fns to globalEnv
}

void RunProfilerOnScript(const std::string& script, std::ostringstream& profilerOutput)
{
    // parsing
    MesaScript_Table scriptEnv;
    __MSRuntime.activeEnv.PushScope(&scriptEnv);
    std::vector<Token> tokens = Lexer(script.c_str());
    auto parser = Parser(tokens);
    parser.parse();

    PLStartProfiling();
    for (auto& scriptTopLevelStatement : parser.scriptExecutionQueue)
    {
        InterpretStatement(scriptTopLevelStatement);
    }
    PLStopProfiling(profilerOutput);

    __MSRuntime.activeEnv.PopScope();
}

void SimplyRunScript(const std::string& script)
{
    MesaScript_Table scriptEnv;
    CompileAndRunMesaScriptCode(script, &scriptEnv);
}

void SimplyRunScriptFromFile(const std::string& pathFromWorkingDir)
{
    std::string fileStr = ReadFileString(wd_path(pathFromWorkingDir).c_str());
    SimplyRunScript(fileStr);
}
