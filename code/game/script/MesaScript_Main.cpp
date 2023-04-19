#include "MesaScript_Main.h"

#include "../../core/CoreCommon.h"
#include "../../core/ArcadiaUtility.h"
#include "../../core/CoreMemoryAllocator.h"

#include <unordered_map>
#include <vector>
#include <map>

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
    True,
    False,
    Identifier,

    FunctionDecl,

    LParen,
    RParen,
    LBrace,
    RBrace,
    Comma,

    If,
    Else,

    Return,
    Print, // temporary
    EndOfLine,
    EndOfFile
};

struct Token
{
    TokenType type = TokenType::Default;
    std::string text;
    u32 startPos = 0;
};

#include "MesaScript_Lexer.cpp"
#include "MesaScript_ASTNodes.cpp"

struct ProcedureDefinition
{
    std::vector<std::string> args; // todo(kevin): could just be a pointer to address in linear allocator with count
    ASTStatementList* body;
};

typedef size_t PID;
#define PID_MAX 256
NiceArray<ProcedureDefinition, PID_MAX> PROCEDURES_DATABASE;
std::vector<ASTProcedureCall*> SCRIPT_PROCEDURE_EXECUTION_QUEUE;

struct TValue
{
    enum class ValueType
    {
        Invalid,
        Integer,
        Real,
        Boolean,
        Function,
        //Table
    };

    union
    {
        i64 integerValue;
        float realValue;
        bool boolValue;
        PID procedureId;
        //MesaScript_Table table;
    };

    ValueType type = ValueType::Invalid;
};

struct CompareFirstChar : public std::binary_function<std::string, std::string, bool>
{
    bool operator()(const std::string& lhs, const std::string& rhs) const
    {
        return lhs.front() < rhs.front();
    }
};

struct MesaScript_Table
{
    // add new pair
    // overwrite existing pair
    // delete existing pair

    //void ArrayAddElement();
    //void ArrayInsert();

    //void ArrayFront();
    //void ArrayBack();

    //size_t ArrayLength()
    //{
    //    return array.size();
    //}

    //TValue& AccessArrayElement(size_t index)
    //{
    //    return array.at(index);
    //    // todo(kevin): out of range error
    //}

    bool TableContainsKey(const std::string& key)
    {
        auto elemIterator = table.find(key);
        return elemIterator != table.end();
    }

    TValue& TableAccessElement(const std::string& key)
    {
        return table.at(key);
    }

    TValue& TableCreateElement(const std::string& key, const TValue value)
    {
        table.emplace(key, value);
        return table.at(key);
    }

private:
    //std::vector<TValue> array;
    std::map<std::string, TValue, CompareFirstChar> table;
};


struct MesaScript_ScriptObject
{
    //TValue& GetAtIndex(size_t index);

    bool KeyExists(const std::string& key)
    {
        for (int back = int(scopes.size()) - 1; back >= 0; --back)
        {
            MesaScript_Table& scope = scopes.at(back);
            if (scope.TableContainsKey(key))
            {
                return true;
            }
        }
        return false;
    }

    TValue& AccessAtKey(const std::string& key)
    {
        for (int back = int(scopes.size()) - 1; back >= 0; --back)
        {
            MesaScript_Table& scope = scopes.at(back);
            if (scope.TableContainsKey(key))
            {   
                return scope.TableAccessElement(key);
            }
        }
    }

    void EmplaceNewElement(std::string key, TValue value)
    {
        scopes.back().TableCreateElement(key, value);
    }

    void PushScope(const MesaScript_Table& scope)
    {
        scopes.push_back(scope);
    }

    void PopScope()
    {
        scopes.pop_back();
    }

private:
    std::vector<MesaScript_Table> scopes;
};


struct MesaScript_Scope
{
    MesaScript_Table GLOBAL_TABLE;
    MesaScript_ScriptObject ACTIVE_SCRIPT_TABLE;
};

static MesaScript_Scope MESASCRIPT_SCOPE;

//static std::unordered_map<std::string, TValue> GLOBAL_SCOPE_SYMBOL_TABLE;

#include "MesaScript_Parser.cpp"
#include "MesaScript_Interpreter.cpp"

#include "../../core/CoreFileSystem.h"

void RunMesaScriptInterpreterOnFile(const char* pathFromWorkingDir)
{
    std::string fileStr = ReadFileString(wd_path(pathFromWorkingDir).c_str());

    MemoryLinearInitialize(&astBuffer, 4096);

    //printf("%ld", sizeof(MesaScript_Table));

    static const char* mesaScriptSetupCode = "fn printv(v) { print v }";

    auto setupTokens = Lexer(std::string(mesaScriptSetupCode));
    auto setupParser = Parser(setupTokens);
    setupParser.parse();

    auto result = Lexer(fileStr.c_str());
    auto parser = Parser(result);
    parser.parse();

    for (auto& procCallNode : SCRIPT_PROCEDURE_EXECUTION_QUEUE)
    {
        InterpretProcedureCall(procCallNode);
    }
}

