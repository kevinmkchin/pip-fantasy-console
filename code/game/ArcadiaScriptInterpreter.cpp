#include "ArcadiaScriptInterpreter.h"

#include "../core/CoreCommon.h"
#include "../core/ArcadiaUtility.h"
#include "../core/CoreMemoryAllocator.h"

#include <unordered_map>
#include <vector>

/*
 * stuff can be one of :
 * - keyword
 *   - if, ret, else, proc, end, int, float, bool, struct, string, while, for, array, list
 * - identifier
 *   - x, y, fib, Start, Update
 * - constant
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
        "ret x \n"
        "else\n"
        "  ret fib(x-1) + fib(x  - 2)\n"
        "end\n"
        //"end\n"
        "";

static const char* script1 = "3 + 7";

enum class TokenType
{
    Default,
    NumberLiteral,
    True,
    False,
    AddOperator,
    SubOperator,
    MulOperator,
    DivOperator,
    LParen,
    RParen,
    EndOfLine,
    EndOfFile
};

struct Token
{
    TokenType type = TokenType::Default;
    std::string text;
    u32 startPos = 0;
};


// char -> bool
// return true if char is one of [0, 9]
bool IsDigit(char c)
{
    return ('0' <= c && c <= '9');
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

    bool flag_NegativeNumberAhead = false;
    while(currentIndex < code.length())
    {
        u32 tokenStartIndex = currentIndex;
        char lookAhead = code.at(currentIndex);
        if(IsWhitespace(lookAhead))
        {
            ++currentIndex;
        }
        else if(lookAhead == '-')
        {
            ++currentIndex;
            retval.push_back({ TokenType::SubOperator, code.substr(tokenStartIndex, currentIndex - tokenStartIndex), tokenStartIndex });
        }
        else if(IsDigit(lookAhead))
        {
            NiceArray<char, 32> numberCharsBuffer;
            numberCharsBuffer.ResetToZero();
            while (currentIndex < code.length() && IsDigit(code.at(currentIndex)))
            {
                numberCharsBuffer.PushBack(code.at(currentIndex));
                ++currentIndex;
            }
            retval.push_back({ TokenType::NumberLiteral, code.substr(tokenStartIndex, currentIndex - tokenStartIndex), tokenStartIndex });
        }
        else if(lookAhead == '\n')
        {
            ++currentIndex;
            retval.push_back({ TokenType::EndOfLine, code.substr(tokenStartIndex, currentIndex - tokenStartIndex), tokenStartIndex });
        }
        else
        {
            ++currentIndex;
            TokenType tokenType = TokenType::Default;
            switch(lookAhead)
            {
                case '+': { tokenType = TokenType::AddOperator; } break;
                case '*': { tokenType = TokenType::MulOperator; } break;
                case '/': { tokenType = TokenType::DivOperator; } break;
                case '(': { tokenType = TokenType::LParen; } break;
                case ')': { tokenType = TokenType::RParen; } break;
                default:{
                    printf("error: unrecognized character in Lexer");
                    continue;
                }
            }
            retval.push_back({ tokenType, code.substr(tokenStartIndex, currentIndex - tokenStartIndex), tokenStartIndex });
        }
    }
    retval.push_back({TokenType::EndOfFile, "<EOF>", (u32)code.length()});
    return retval;
}



class ASTNode
{

};

class ASTAssignment : public ASTNode
{
public:
    ASTAssignment();
public:
    ASTNode* id;
    ASTNode* expr;
};

class ASTWhile : public ASTNode
{
public:
    ASTWhile();
public:
    ASTNode* condition;
    ASTNode* body;
};

class ASTNumberTerminal : public ASTNode
{
public:
    ASTNumberTerminal(i32 num);
public:
    i32 value;
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
public:
    BinOp op; // add, sub, mul, div
    ASTNode* left;
    ASTNode* right;
};


//
//    // a = 2 + 3 * -7 - 1
//
//  
//
//    //     assign(id(a), rhs)
//    // assign a b
//
//    (2 + (3 * -7)) - 1

// 


static MemoryLinearBuffer astBuffer;

class Parser
{
public:
    Parser(std::vector<Token> _tokens);

    ASTNode* parse();

private:
    void error();

    void eat(TokenType tpe);

    ASTNode* factor();

    ASTNode* term();

    ASTNode* expr();


private:
    std::vector<Token> tokens;
    Token currentToken;
    size_t currentTokenIndex;
};

Parser::Parser(std::vector<Token> _tokens)
    : tokens(_tokens)
    , currentToken(tokens.front())
    , currentTokenIndex(0)
{}

ASTNode* Parser::parse()
{
    return expr();
}

void Parser::error()
{
    printf("oof\n");
    ASSERT(0);
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
        error();
    }
}

ASTNode* Parser::factor()
{
    // factor : NUMBER | LPAREN expr RPAREN | IDENTIFIER

    auto t = currentToken;

    if (t.type == TokenType::NumberLiteral)
    {
        eat(TokenType::NumberLiteral);
        ASTNode* node = 
            new(MemoryLinearAllocate(&astBuffer, sizeof(ASTNumberTerminal), 8)) 
            ASTNumberTerminal(atoi(t.text.c_str()));
        return node;
    }
    else if (t.type == TokenType::LParen)
    {
        eat(TokenType::LParen);
        auto node = expr();
        eat(TokenType::RParen);
        return node;
    }
    else
    {
        error();
    }
}

ASTNode* Parser::term()
{
    // term : factor ((MUL | DIV) factor)*

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
            new(MemoryLinearAllocate(&astBuffer, sizeof(ASTBinOp), 8))
            ASTBinOp(op, node, factor());
    }

    return node;
}

ASTNode* Parser::expr()
{
    // expr : term ((PLUS | MINUS) term)*

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
            new(MemoryLinearAllocate(&astBuffer, sizeof(ASTBinOp), 8))
            ASTBinOp(op, node, term());
    }

    return node;
}


ASTNumberTerminal::ASTNumberTerminal(i32 num)
    : value(num)
{}

ASTBinOp::ASTBinOp(BinOp op, ASTNode* left, ASTNode* right)
    : op(op)
    , left(left)
    , right(right)
{}



// would be nice to have polymorphism
// give me x amount of memory
// use linear allocator bcs x memory will never be dealloced

void TestProc()
{
    MemoryLinearInitialize(&astBuffer, 4096);

    //auto result = Lexer(" 7 + 42 ");
    auto result = Lexer(" 2 +3* 7 - 1 ");
    auto parser = Parser(result);
    auto v = parser.parse();

    // void* a = MemoryLinearAllocate(&astBuffer, 16, 16);
    // void* b = MemoryLinearAllocate(&astBuffer, 32, 32);
    // void* c = MemoryLinearAllocate(&astBuffer, 16, 16);
    // void* d = MemoryLinearAllocate(&astBuffer, 128, 16);
    // ASTNode* l = new(MemoryLinearAllocate(&astBuffer, sizeof(ASTNumberTerminal), 8)) ASTNumberTerminal(78);
    // ASTNode* r = new(MemoryLinearAllocate(&astBuffer, sizeof(ASTNumberTerminal), 8)) ASTNumberTerminal(42);
    // ASTBinOp* op = new(MemoryLinearAllocate(&astBuffer, sizeof(ASTBinOp), 8)) ASTBinOp(BinOp::Add);
    // op->left = l;
    // op->right = r;

    printf("hello\n");
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
//std::string GetLine(std::string* code)
//{
//    size_t eol = code->find_first_of('\n');
//    std::string line;
//    if (eol == std::string::npos)
//    {
//        line = *code;
//        code->clear();
//    }
//    line = code->substr(0, eol);
//    *code = code->substr(eol + 1);
//    return line;
//}
//
//std::string GetToken(std::string* code)
//{
//
//    // if ( x<= (4 + 3)) ...
//    // 1 - "if"
//    // 2 - "x<= (4 + 3)"
//
//    // x<= (4 + 3)
//    // 1 - x
//    // 2 - <=
//    // 3 - 4 + 3
//
//}
//
//void ExecuteProcedure(proc_id pid)
//{
//    // before executing procedure, the prior procedure should place function arguments
//    // on the stack and move the base pointer above the function arguments
//
//    proc_definition proc = procedure_definitions.At(pid);
//    std::string code = proc.proc_code;
//
//    while (!code.empty())
//    {
//        std::string line = GetLine(&code);
//        if (line.empty()) continue;
//
//        // go through line and replace all identifiers with their constant value
//        // go through line and reduce operations to constant value
//        //   e.g. false or 3 <= (4 + 2)  -->  true
//        //        6 + 3 * 2  -->  12
//
//
//        std::string token = GetToken(&line);
//        if (token == "if")
//        {
//            // evaluate condition
//            // if true then look for else/end and do everything before it
//            // if false then look for else (if exist, then skip to else)
//            //   if found end instead, then skip to end
//        }
//        else if (token == "else")
//        {
//
//        }
//        else if (token == "ret")
//        {
//
//        }
//        else if (token == "<")
//        {
//
//        }
//        else
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
//    // if token is "if" then check condition, if condition is false, skip all code
//    code.get
//}
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
