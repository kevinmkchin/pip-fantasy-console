#include "MesaScript.h"

#include "MesaCommon.h"
#include "MemoryAllocator.h"
#include "FileSystem.h"

#include <cmath>
#include <string>

#pragma region StaticVariables

struct ProcedureDefinition
{
    std::vector<std::string> args; // todo(kevin): could just be a pointer to address in linear allocator with count
    class ASTStatementList* body;
};

NiceArray<ProcedureDefinition, PID_MAX> PROCEDURES_DATABASE;

u64 gcobjIdTicker = 1; // 0 is invalid
std::unordered_map<u64, MesaGCObject*> GCOBJECTS_DATABASE;

struct MesaScriptRuntimeObject
{
    MesaScript_Table              globalEnv;
    MesaScript_ScriptEnvironment *activeEnv = NULL;
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


MesaGCObject::GCObjectType GetTypeOfGCObject(u64 gcObjectId)
{
    MesaGCObject* gcobj = GCOBJECTS_DATABASE.at(gcObjectId);
    return gcobj->GetType();
}

MesaScript_Table* AccessMesaScriptTable(u64 gcObjectId)
{
    return (MesaScript_Table*)(GCOBJECTS_DATABASE.at(gcObjectId));
}

/// Simply returns the MesaScript_List associated with the given GCObject id.
MesaScript_List* AccessMesaScriptList(u64 gcObjectId)
{
    return (MesaScript_List*)(GCOBJECTS_DATABASE.at(gcObjectId));
}

/// Simply returns the MesaScript_String associated with the given GCObject id.
MesaScript_String* AccessMesaScriptString(u64 gcObjectId)
{
    return (MesaScript_String*)(GCOBJECTS_DATABASE.at(gcObjectId));
}

void IncrementReferenceGCObject(u64 gcObjectId)
{
    MesaGCObject* gcobj = GCOBJECTS_DATABASE.at(gcObjectId);
    gcobj->refCount++;

    if (__MSRuntime.activeEnv->TransientObjectExists(gcObjectId))
    {
        __MSRuntime.activeEnv->EraseTransientObject(gcObjectId);
        if (gcobj->refCount <= 0) ReleaseReferenceGCObject(gcObjectId);
    }
}

void ReleaseReferenceGCObject(u64 gcObjectId)
{
    MesaGCObject* gcobj = GCOBJECTS_DATABASE.at(gcObjectId);
    gcobj->refCount--;
    if (gcobj->refCount <= 0)
    {
        if (__MSRuntime.activeEnv->TransientObjectExists(gcObjectId)) return;

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
        GCOBJECTS_DATABASE.erase(gcObjectId);
    }
}

u64 RequestNewGCObject(MesaGCObject::GCObjectType gcObjectType)
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

void MesaScript_Table::CreateNewMapEntry(const std::string& key, const TValue value)
{
    if (value.type == TValue::ValueType::GCObject)
    {
        IncrementReferenceGCObject(value.GCReferenceObject);
    }

    table.emplace(key, value);
}

void MesaScript_Table::ReplaceMapEntryAtKey(const std::string& key, const TValue value)
{
    TValue existingValue = table.at(key);
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

    table.at(key) = value;
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

void SetActiveScriptEnvironment(MesaScript_ScriptEnvironment* env)
{
    __MSRuntime.activeEnv = env;
}

int ActiveScopeDepthIndex()
{
    return (int) __MSRuntime.activeEnv->ScopesSize() - 1;
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
        else if (lookAhead == '~') // line comments
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
                retval.push_back({ TokenType::FunctionDecl,code.substr(tokenStartIndex, currentIndex - tokenStartIndex), tokenStartIndex, currentLine });
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
                case ';': { continue; } break;
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
    ASTWhile();

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
    std::vector<ASTProcedureCall *> temporaryProcedureExecutionQueue;
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
        else if (currentToken.type == TokenType::Identifier)
        {
            temporaryProcedureExecutionQueue.push_back(static_cast<ASTProcedureCall*>(procedure_call()));
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

    if (__MSRuntime.activeEnv->KeyExists(procedureNameToken.text))
    {
        __MSRuntime.activeEnv->ReplaceAtKey(procedureNameToken.text, functionVariable);
    }
    else
    {
        __MSRuntime.activeEnv->EmplaceNewElement(procedureNameToken.text, functionVariable);
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
    TValue result;

    switch(ast->GetType())
    {
        case ASTNodeType::SIMPLYTVALUE: 
        {
            auto v = static_cast<ASTSimplyTValue*>(ast);
            result = v->value;
            break;
        }
        case ASTNodeType::BINOP:
        {
            auto v = static_cast<ASTBinOp*>(ast);
            TValue l = InterpretExpression(v->left);
            TValue r = InterpretExpression(v->right);
            // both integer, then integer
            // both float, then float
            // one int, one float, then float
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
            break;
        }
        
        case ASTNodeType::RELOP:
        {
            auto v = static_cast<ASTRelOp*>(ast);
            TValue l = InterpretExpression(v->left);
            TValue r = InterpretExpression(v->right);
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
            auto v = static_cast<ASTVariable*>(ast);
            if (__MSRuntime.activeEnv->KeyExistsInCurrentScope(v->id))
            {
                result = __MSRuntime.activeEnv->AccessAtKey(v->id);
            }
            else if (__MSRuntime.globalEnv.Contains(v->id))
            {
                result = __MSRuntime.globalEnv.AccessMapEntry(v->id);
            }
            else
            {
                SendRuntimeException("Unknown identifier in expression.");
            }
            break;
        }

        case ASTNodeType::ACCESS_LIST_OR_MAP_ELEMENT:
        {
            auto v = static_cast<ASTAccessListOrMapElement*>(ast);

            auto indexTValue = InterpretExpression(v->indexExpression);
            ASSERT(indexTValue.type == TValue::ValueType::Integer || indexTValue.type == TValue::ValueType::GCObject);

            ASSERT(v->listOrMapVariableName->GetType() == ASTNodeType::VARIABLE);
            std::string listOrMapVariableKey = static_cast<ASTVariable*>(v->listOrMapVariableName)->id;

            u64 gcObjectId = 0;
            if (__MSRuntime.activeEnv->KeyExistsInCurrentScope(listOrMapVariableKey))
            {
                TValue listOrMapGCObj = __MSRuntime.activeEnv->AccessAtKey(listOrMapVariableKey);
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
            break;
        }

        case ASTNodeType::CREATETABLE:
        {
            //auto v = static_cast<CreateNewMapEntry*>(ast);
            result.type = TValue::ValueType::GCObject;
            result.GCReferenceObject = RequestNewGCObject(MesaGCObject::GCObjectType::Table);
            break;
        }

        case ASTNodeType::CREATELIST:
        {
            auto v = static_cast<ASTCreateList*>(ast);
            result.type = TValue::ValueType::GCObject;
            result.GCReferenceObject = RequestNewGCObject(MesaGCObject::GCObjectType::List);
            MesaScript_List* mesaList = AccessMesaScriptList(result.GCReferenceObject);
            __MSRuntime.activeEnv->InsertTransientObject(result.GCReferenceObject, 0);
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
            break;
        }
        case ASTNodeType::STRING:
        {
            auto v = static_cast<ASTStringTerminal*>(ast);
            result.type = TValue::ValueType::GCObject;
            result.GCReferenceObject = RequestNewGCObject(MesaGCObject::GCObjectType::String);
            MesaScript_String *createdString = AccessMesaScriptString(result.GCReferenceObject);
            createdString->text = v->value;
            __MSRuntime.activeEnv->InsertTransientObject(result.GCReferenceObject, 0);
            break;
        }
        case ASTNodeType::BOOLEAN:
        {
            auto v = static_cast<ASTBooleanTerminal*>(ast);
            result.boolValue = v->value;
            result.type = TValue::ValueType::Boolean;
            break;
        }
        case ASTNodeType::PROCEDURECALL:
        {
            auto v = static_cast<ASTProcedureCall*>(ast);
            result = InterpretProcedureCall(v);
            break;
        }
    }

    if (result.type == TValue::ValueType::Invalid)
    {
        SendRuntimeException("Invalid expression.");
    }
    else if (result.type == TValue::ValueType::GCObject
        && GetTypeOfGCObject(result.GCReferenceObject) == MesaGCObject::GCObjectType::String
        && !__MSRuntime.activeEnv->TransientObjectExists(result.GCReferenceObject))
    {
        i64 stringCopyId = RequestNewGCObject(MesaGCObject::GCObjectType::String);
        AccessMesaScriptString(stringCopyId)->text = AccessMesaScriptString(result.GCReferenceObject)->text;
        result.GCReferenceObject = stringCopyId;
        __MSRuntime.activeEnv->InsertTransientObject(stringCopyId, 0);
    }

    return result;
}

static void
InterpretStatement(ASTNode* statement)
{
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
            if (__MSRuntime.activeEnv->KeyExistsInCurrentScope(key))
            {
                __MSRuntime.activeEnv->ReplaceAtKey(key, result);
            }
            else if (__MSRuntime.globalEnv.Contains(key))
            {
                __MSRuntime.globalEnv.ReplaceMapEntryAtKey(key, result);
            }
            else
            {
                __MSRuntime.activeEnv->EmplaceNewElement(key, result);
            }
        } break;
        case ASTNodeType::ASSIGN_LIST_OR_MAP_ELEMENT: {
            auto v = static_cast<ASTAssignListOrMapElement*>(statement);

            auto indexTValue = InterpretExpression(v->indexExpression);
            ASSERT(indexTValue.type == TValue::ValueType::Integer || indexTValue.type == TValue::ValueType::GCObject);

            ASSERT(v->listOrMapVariableName->GetType() == ASTNodeType::VARIABLE);
            std::string listOrMapVariableKey = static_cast<ASTVariable*>(v->listOrMapVariableName)->id;

            u64 gcObjectId = 0;
            if (__MSRuntime.activeEnv->KeyExistsInCurrentScope(listOrMapVariableKey))
            {
                TValue listOrMapGCObj = __MSRuntime.activeEnv->AccessAtKey(listOrMapVariableKey);
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
    }

    // ALL STATEMENTS MUST REACH THIS POINT
    __MSRuntime.activeEnv->ClearTransientsInTopLevelScope();
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

TValue CPPBOUND_MESASCRIPT_Print(TValue value)
{
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
                    printf("    %s : %lld\n", pair.first.c_str(), pair.second.integerValue);
                }
                else if (pair.second.type == TValue::ValueType::Boolean)
                {
                    printf("    %s : %s\n", pair.first.c_str(), (pair.second.boolValue ? "true" : "false"));
                }
                else if (pair.second.type == TValue::ValueType::Real)
                {
                    printf("    %s : %lf\n", pair.first.c_str(), pair.second.realValue);
                }
                else if (pair.second.type == TValue::ValueType::GCObject)
                {
                    printf("    %s : gcobject\n", pair.first.c_str());
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
                    printf("    %lld\n", element.integerValue);
                }
                else if (element.type == TValue::ValueType::Boolean)
                {
                    printf("    %s\n", (element.boolValue ? "true" : "false"));
                }
                else if (element.type == TValue::ValueType::Real)
                {
                    printf("    %lf\n", element.realValue);
                }
                else if (element.type == TValue::ValueType::GCObject)
                {
                    printf("    gcobject\n");
                }
            }  
        }
        else if (GetTypeOfGCObject(value.GCReferenceObject) == MesaGCObject::GCObjectType::String)
        {
            printf("%s\n", AccessMesaScriptString(value.GCReferenceObject)->text.c_str());
        }
    }

    return TValue();
}

TValue CPPBOUND_MESASCRIPT_RaiseTo(TValue base, TValue exponent)
{
    ASSERT(base.type == TValue::ValueType::Integer); // todo can be float too
    ASSERT(exponent.type == TValue::ValueType::Integer);

    TValue result;
    result.type = TValue::ValueType::Integer;
    result.integerValue = (int)pow(double(base.integerValue), double(exponent.integerValue));
    return result;
}

TValue CPPBOUND_MESASCRIPT_RefCount(TValue gcobj)
{
    ASSERT(gcobj.type == TValue::ValueType::GCObject);

    TValue result;
    result.type = TValue::ValueType::Integer;
    result.integerValue = (int)GCOBJECTS_DATABASE.at(gcobj.GCReferenceObject)->refCount;
    return result;
}


static TValue
InterpretProcedureCall(ASTProcedureCall *procedureCall) // NEVER CALL UNLESS PART OF INTERPRETER, USE INTERPRETSTATEMENT INSTEAD
{
    // If ProcedureCall is C++ bound function, then call that.
    // TValue result = cpp_fn(InterpretExpression(procedureCall->argsExpressions[0]), InterpretExpression(procedureCall->argsExpressions[1]));

    TValue procedureVariable;
    if (__MSRuntime.activeEnv->KeyExists(procedureCall->id))
    {
        procedureVariable = __MSRuntime.activeEnv->AccessAtKey(procedureCall->id);
    }
    else if (__MSRuntime.globalEnv.Contains(procedureCall->id))
    {
        procedureVariable = __MSRuntime.globalEnv.AccessMapEntry(procedureCall->id);
    }
    else // only reach this case if valid CPPBOUND function or unknown function identifier error
    {
        if (procedureCall->id == "raise_to")
        {
            ASSERT(procedureCall->argsExpressions.size() == 2);
            return CPPBOUND_MESASCRIPT_RaiseTo(InterpretExpression(procedureCall->argsExpressions[0]), InterpretExpression(procedureCall->argsExpressions[1]));
        }
        else if (procedureCall->id == "print")
        {
            ASSERT(procedureCall->argsExpressions.size() == 1);
            return CPPBOUND_MESASCRIPT_Print(InterpretExpression(procedureCall->argsExpressions[0]));
        }
        else if (procedureCall->id == "refcount")
        {
            ASSERT(procedureCall->argsExpressions.size() == 1);
            return CPPBOUND_MESASCRIPT_RefCount(InterpretExpression(procedureCall->argsExpressions[0]));
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
    __MSRuntime.activeEnv->PushScope(functionScope);

    TValue retval;
    returnRequestedFlag = false;
    returnValueSetFlag = false;

    InterpretStatementList(procedureDefinition.body);

    if (returnValueSetFlag) 
    {
        retval = returnValue;

        if (returnValue.type == TValue::ValueType::GCObject)
        {
            __MSRuntime.activeEnv->InsertTransientObject(returnValue.GCReferenceObject, -1);
        }
    }

    returnRequestedFlag = false;
    returnValueSetFlag = false;

    // Intended: activeEnv->PopScope should decrement ref count of every entry of the function scope being popped.
    __MSRuntime.activeEnv->PopScope();

    return retval;
}

#pragma endregion Interpreter

MesaScript_ScriptEnvironment CompileEntityBehaviourAsNewScriptEnvironment(const std::string& entityBehaviourScript)
{
    MesaScript_ScriptEnvironment scriptEnvironment;
    scriptEnvironment.PushScope(MesaScript_Table());
    SetActiveScriptEnvironment(&scriptEnvironment);

    std::vector<Token> tokens = Lexer(entityBehaviourScript.c_str());
    auto parser = Parser(tokens);
    parser.parse();
    // TODO(Kevin): if not release mode, probably want to keep Parser paired with the ScriptEnvironment so we have debug data later

    SetActiveScriptEnvironment(NULL);
    return scriptEnvironment;
}

void CallParameterlessFunctionInActiveScriptEnvironment(const char* functionIdentifier)
{
    ASTProcedureCall parameterlessProcCallNode = ASTProcedureCall(functionIdentifier);
    InterpretStatement(&parameterlessProcCallNode);
}

void CallFunctionInActiveScriptEnvironmentWithOneParam(const char* functionIdentifier, TValue arg0)
{
    ASTProcedureCall procCallNode = ASTProcedureCall(functionIdentifier);
    ASTSimplyTValue arg0Node = ASTSimplyTValue(arg0);
    procCallNode.argsExpressions.push_back(&arg0Node);
    InterpretStatement(&procCallNode);
}

void InitializeLanguageCompilerAndRuntime()
{
    MemoryLinearInitialize(&__ASTBuffer, 8000000);
}

// TODO(Kevin): move initialize and setup stuff somewhere else. make sure built-in methods like add get added to global scope not script scope.
void TemporaryRunMesaScriptInterpreterOnFile(const std::string& pathFromWorkingDir)
{
    std::string fileStr = ReadFileString(wd_path(pathFromWorkingDir).c_str());

    //printf("%ld", sizeof(MesaScript_Table));

    MesaScript_ScriptEnvironment scriptEnvironment;
    scriptEnvironment.PushScope(MesaScript_Table());
    SetActiveScriptEnvironment(&scriptEnvironment);

    static const char* mesaScriptSetupCode = "fn add(x, y) { return x + y } fn checkeq(expected, actual) { if (expected == actual) { print('test pass') } else { print('test fail') } }";

    auto setupTokens = Lexer(std::string(mesaScriptSetupCode));
    auto setupParser = Parser(setupTokens);
    setupParser.parse();

    auto result = Lexer(fileStr.c_str());
    auto parser = Parser(result);
    parser.parse();

    for (auto& procCallNode : parser.temporaryProcedureExecutionQueue)
    {
        InterpretStatement(procCallNode);
    }
}
