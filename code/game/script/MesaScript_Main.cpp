#include "MesaScript_Main.h"

#include "../../core/CoreCommon.h"
#include "../../core/ArcadiaUtility.h"
#include "../../core/CoreMemoryAllocator.h"

#include <unordered_map>
#include <vector>

/** TODO
    - procedure return values
    - procedure arguments
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

// returns the x-th fibonnaci number
static const char* script0 =
        //"proc fib(int x)\n"
        "if (x< 2) \n"
        "  return x \n"
        "else\n"
        "  return fib(x-1) + fib(x  - 2)\n"
        "\n"
        //"end\n"
        "";

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

typedef size_t PID;
#define PID_MAX 256
NiceArray<ASTStatementList*, PID_MAX> PROCEDURES_DATABASE;

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
    auto result = Lexer(" "
                        "A(){ "
                        "  x = B"
                        "  x()"
                        "  print x"
                        "}"
                        ""
                        "B(){"
                        "  x = 42"
                        "}"
                        "");
    auto parser = Parser(result);
    parser.parse();
    InterpretStatementList(PROCEDURES_DATABASE.At((unsigned int)GLOBAL_SCOPE_SYMBOL_TABLE.at("A").procedureId));

}


// need database of function definitions
// need a stack
//  need base pointer
//  need types and their sizes

//static char stack[4096];
//static u32 base_ptr = 0;
//static u32 stack_ptr = 0;
//
//typedef u32 proc_id;
//
//struct proc_definition
//{
//    std::string proc_code;
//    u8 arg_count;
//    u8 arg_size;
//    u8 retval_size;
//};
//
//std::unordered_map<std::string, u32> pname_to_pid;
//NiceArray<proc_definition, 4> procedure_definitions;
//
//
//void ExecuteProcedure(proc_id pid)
//{
//    // before executing procedure, the prior procedure should place function arguments
//    // on the stack and move the base pointer above the function arguments
//
//    proc_definition proc = procedure_definitions.At(pid);
//    std::string code = proc.proc_code;
//
//        {
//            u32 processToCall = pname_to_pid["fib"];
//
//            u32 sizeOfLocalsOfStackFrame = stack_ptr - base_ptr;
//            base_ptr += sizeOfLocalsOfStackFrame + proc.arg_size;
//            stack_ptr = base_ptr;
//            ExecuteProcedure(processToCall);
//            u32 sizeOfCalledProcedureLocals = stack_ptr - base_ptr;
//            stack_ptr = stack_ptr - sizeOfCalledProcedureLocals - proc.arg_size;
//            base_ptr = base_ptr - sizeOfLocalsOfStackFrame - proc.arg_size;
//        }
//    }
//
//void InterpretScript0()
//{
//    proc_definition proc = { script0, 1, 4, 4 };
//    procedure_definitions.PushBack(proc);
//    pname_to_pid.emplace("fib", 0);
//
//    u32 sizeOfLocalsOfStackFrame = stack_ptr - base_ptr;
//    base_ptr += sizeOfLocalsOfStackFrame + proc.arg_size;
//    stack_ptr = base_ptr;
//    ExecuteProcedure(0);
//    u32 sizeOfCalledProcedureLocals = stack_ptr - base_ptr;
//    stack_ptr = stack_ptr - sizeOfCalledProcedureLocals - proc.arg_size;
//    base_ptr = base_ptr - sizeOfLocalsOfStackFrame - proc.arg_size;
//
//
//}
