#include "MesaScript_Main.h"

#include "../../core/CoreCommon.h"
#include "../../core/ArcadiaUtility.h"
#include "../../core/CoreMemoryAllocator.h"

#include <unordered_map>
#include <vector>

/** TODO
    - scopes and symbol tables

    - floats

    - elif
    - while
    - for

    - use custom assert for mesascript
    - replace all std::vectors with custom data struct
    - REFACTOR
*/

/*
 * stuff can be one of :
 * - keyword
 *   - if, ret, else, proc, end, int, float, bool, struct, string, while, for, array, list
 * - identifier
 *   - x, y, fib, Start, Update
 * - terminal
 *   - 2, 514, 3.14, "hello world", true, false
 * - operator
 *   - +, -, *, /, <, >, is, <=, >=, isnt, and, or
 *   - = assignment operator
 *
 * */

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

struct TValue
{
    enum class ValueType
    {
        Invalid,
        Integer,
        Real,
        Boolean,
        Function
    };

    union
    {
        i64 integerValue;
        float realValue;
        bool boolValue;
        PID procedureId;
    };

    ValueType type = ValueType::Invalid;
};

static std::unordered_map<std::string, TValue> GLOBAL_SCOPE_SYMBOL_TABLE;

#include "MesaScript_Parser.cpp"
#include "MesaScript_Interpreter.cpp"


void TestProc()
{
    MemoryLinearInitialize(&astBuffer, 4096);

    //auto result = Lexer("return 7 - (4 + 3) ");
    //auto result = Lexer(" return 2 +3*-7 - 1 ");
    //auto result = Lexer(" x = 2 + 3 * y - z\n  return x");
    //auto result = Lexer(" x = (3 >= y) and (2 == 4 or true)\n  return x");
    //auto result = Lexer("((false)) or 4 + y > 7 - (4 + 3) and 45 != z\n"
    //                    "x = (3 >= y) and (2 == 4 or true)\n"
    //                    "return !(4 < (3 + 2)) and false");
    //                    "return x");
    //auto result = Lexer("if false return x else if false return y else if true return z");
    //auto result = Lexer("return ! (4 < (3 + 2)) and false");

    //auto result = Lexer(" return -170*3*5+4-2+1");
    // if x < 3 { do_x do_y do_z }
//    auto result = Lexer(" "
//                        "if (4 < 32) "
//                        "   if (false) "
//                        "       return 3 "
//                        "   else "
//                        "       return 7 "
//                        "else "
//                        "   return -2 - 4 ");
//    auto result = Lexer(""
//                        "x = false  "
//                        "y = 3 + 7 * 32   "
//                        "if(x) "
//                        "   return x "
//                        "else "
//                        "   return y");
//    auto result = Lexer(" "
//                        "{ "
//                        "   x = 11  "
//                        "   if false "
//                        "   { "
//                        "       y = 3   "
//                        "       x = y "
//                        "   } "
//                        "   else "
//                        "   { "
//                        "       y = 9  "
//                        "       x = x + y "
//                        "   }  "
//                        "   return x "
//                        "} ");
//    auto result = Lexer(" "
//                        "A(){ "
//                        "  x = B"
//                        "  print x()"
//                        "}"
//                        ""
//                        "B(){"
//                        "  x = 42"
//                        "  return x + 80"
//                        "}"
//                        "");
//    auto result = Lexer(" "
//                        "A(){ "
//                        "  x = square"
//                        "  x = x(6)"
//                        "  print x"
//                        "}"
//                        ""
//                        "square(n){"
//                        "  return n*n"
//                        "}"
//                        "");

    // returns n-th fibonnacci number
    static const char* script0 = ""
                                 "fib (n) {"
                                 "  if (n < 2) {"
                                 "    return n"
                                 "  } else {"
                                 "    return fib(n - 1) + fib(n - 2)"
                                 "  }"
                                 "}"
                                 ""
                                 "call_fib () {"
                                 "  print fib(9)"
                                 "}"
                                 ""
                                 "";

    auto result = Lexer(script0);
    auto parser = Parser(result);
    parser.parse();
    ASTProcedureCall pcall = ASTProcedureCall("call_fib");
//    ASTNumberTerminal pargnum = ASTNumberTerminal(3);
//    pcall.argsExpressions.push_back(&pargnum);
    InterpretProcedureCall(&pcall);

}

