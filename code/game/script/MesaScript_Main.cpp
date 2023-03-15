#include "MesaScript_Main.h"

#include "../../core/CoreCommon.h"
#include "../../core/ArcadiaUtility.h"
#include "../../core/CoreMemoryAllocator.h"

#include <unordered_map>
#include <vector>

/** TODO
    - ast interpreter start work
    - print (or just use return as print for now)
    - while
    - for

    - scopes and symbol tables
    - floats
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

    If,
    Else,

    Return,
    EndOfLine,
    EndOfFile
};

//if b statementx
//if b statementx else statementy
//if b (statementx) else (if c (statementy) else (if d statementz else (statementa)))
//if b
//    statementx
//else if c
//    statementy
//else if d
//    statementz
//else
//    statementa
// maybe we only need one "end"?

bool IsValueType(TokenType type)
{
    return 
        type == TokenType::NumberLiteral ||
        type == TokenType::Identifier ||
        type == TokenType::RParen;
}

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
            if (!IsValueType(retval.back().type) && currentIndex < code.length() && IsDigit(code.at(currentIndex)))
            {
                flag_NegativeNumberAhead = true;
            }
            else
            {
                retval.push_back({ TokenType::SubOperator, code.substr(tokenStartIndex, currentIndex - tokenStartIndex), tokenStartIndex });
            }
        }
        else if(lookAhead == '<' || lookAhead == '>' || lookAhead == '!' || lookAhead == '=')
        {
            ++currentIndex;
            if (currentIndex < code.length() && code.at(currentIndex) == '=') // check if next char is =
            {
                ++currentIndex;
                if (lookAhead == '<')
                {
                    retval.push_back({ TokenType::LessThanOrEqual, code.substr(tokenStartIndex, currentIndex - tokenStartIndex), tokenStartIndex });
                }
                else if (lookAhead == '>')
                {
                    retval.push_back({ TokenType::GreaterThanOrEqual, code.substr(tokenStartIndex, currentIndex - tokenStartIndex), tokenStartIndex });
                }
                else if (lookAhead == '!')
                {
                    retval.push_back({ TokenType::NotEqual, code.substr(tokenStartIndex, currentIndex - tokenStartIndex), tokenStartIndex });
                }
                else if (lookAhead == '=')
                {
                    retval.push_back({ TokenType::Equal, code.substr(tokenStartIndex, currentIndex - tokenStartIndex), tokenStartIndex });
                }
            }
            else if (lookAhead == '<')
            {
                retval.push_back({ TokenType::LessThan, code.substr(tokenStartIndex, currentIndex - tokenStartIndex), tokenStartIndex });
            }
            else if (lookAhead == '>')
            {
                retval.push_back({ TokenType::GreaterThan, code.substr(tokenStartIndex, currentIndex - tokenStartIndex), tokenStartIndex });
            }
            else if (lookAhead == '=')
            {
                retval.push_back({ TokenType::AssignmentOperator, code.substr(tokenStartIndex, currentIndex - tokenStartIndex), tokenStartIndex });
            }
            else if (lookAhead == '!')
            {
                retval.push_back({TokenType::LogicalNegation, code.substr(tokenStartIndex, currentIndex - tokenStartIndex), tokenStartIndex });
            }
        }
        else if(IsDigit(lookAhead))
        {
            if (flag_NegativeNumberAhead)
            {
                tokenStartIndex -= 1;
                flag_NegativeNumberAhead = false;
            }
            while (currentIndex < code.length() && IsDigit(code.at(currentIndex)))
            {
                ++currentIndex;
            }
            retval.push_back({ TokenType::NumberLiteral, code.substr(tokenStartIndex, currentIndex - tokenStartIndex), tokenStartIndex });
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

            // check if word is a keyword
            // - while
            // - if else
            // - for
            // - continue
            // - break
            // - int
            // - float
            // - bool
            // - string
            // - struct
            auto word = std::string(wordCharsBuffer.data);
            if (word == "return")
            {
                retval.push_back({ TokenType::Return, code.substr(tokenStartIndex, currentIndex - tokenStartIndex), tokenStartIndex });
            }
            else if (word == "true")
            {
                retval.push_back({ TokenType::True, code.substr(tokenStartIndex, currentIndex - tokenStartIndex), tokenStartIndex });
            }
            else if (word == "false")
            {
                retval.push_back({ TokenType::False, code.substr(tokenStartIndex, currentIndex - tokenStartIndex), tokenStartIndex });
            }
            else if (word == "and")
            {
                retval.push_back({TokenType::LogicalAnd, code.substr(tokenStartIndex, currentIndex - tokenStartIndex), tokenStartIndex });
            }
            else if (word == "or")
            {
                retval.push_back({TokenType::LogicalOr, code.substr(tokenStartIndex, currentIndex - tokenStartIndex), tokenStartIndex });
            }
            else if (word == "if")
            {
                retval.push_back({ TokenType::If, code.substr(tokenStartIndex, currentIndex - tokenStartIndex), tokenStartIndex });
            }
            else if (word == "else")
            {
                retval.push_back({ TokenType::Else, code.substr(tokenStartIndex, currentIndex - tokenStartIndex), tokenStartIndex });
            }
            else // otherwise, word is function call or identifier
            {
                retval.push_back({ TokenType::Identifier, code.substr(tokenStartIndex, currentIndex - tokenStartIndex), tokenStartIndex });
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
                case '\n': { continue; /*tokenType = TokenType::EndOfLine;*/ } break;
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


#include "MesaScript_ASTNodes.cpp"


static MemoryLinearBuffer astBuffer;

class Parser
{
public:
    Parser(std::vector<Token> _tokens);

    std::vector<ASTNode*> parse();

private:
    void error();

    void eat(TokenType tpe);

    ASTNode* factor();
    ASTNode* term();
    ASTNode* expr();

    ASTNode* cond_expr();
    ASTNode* cond_equal();
    ASTNode* cond_and();
    ASTNode* cond_or();

    ASTNode* statement();


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

std::vector<ASTNode*> Parser::parse()
{
    std::vector<ASTNode*> statementSequence;

    while(currentTokenIndex < tokens.size() - 1)
    {
        statementSequence.push_back(statement());
        //eat(TokenType::EndOfLine);
    }

    return statementSequence;
}

ASTNode* Parser::statement()
{
    // statement : IDENTIFIER ASSIGN cond_or
    // statement : RETURN cond_or
    // statement : expr  // this is valid because a statement can be a function call
    // statement : IF cond_or statement (ELSE statement)? // todo change statement to statement sequence
    // todo : WHILE cond_or (statement sequence)

    if (currentToken.type == TokenType::Identifier)
    {
        auto t = currentToken;
        eat(TokenType::Identifier);
        eat(TokenType::AssignmentOperator);
        auto varNode = 
            new (MemoryLinearAllocate(&astBuffer, sizeof(ASTVariable), 32))
            ASTVariable(t.text);
        auto node = 
            new (MemoryLinearAllocate(&astBuffer, sizeof(ASTAssignment), 8))
            ASTAssignment(varNode, cond_or());
        return node;
    }
    else if (currentToken.type == TokenType::Return)
    {
        eat(TokenType::Return);
        auto node =
            new (MemoryLinearAllocate(&astBuffer, sizeof(ASTReturn), 8))
            ASTReturn(cond_or());
        return node;
    }
    else if (currentToken.type == TokenType::If)
    {
        eat(TokenType::If);
        ASTNode* condition = cond_or();
        ASTNode* ifCase = statement(); // todo sequence of statements
        ASTNode* elseCase = nullptr;
        if (currentToken.type == TokenType::Else)
        {
            eat(TokenType::Else);
            elseCase = statement(); // todo sequence of statements
        }

        auto node =
                new (MemoryLinearAllocate(&astBuffer, sizeof(ASTBranch), 8))
                ASTBranch(condition, ifCase, elseCase);
        return node;
    }
    else
    {
        return cond_or();
    }
}

ASTNode* Parser::cond_expr()
{
    // cond_expr : expr ((< | > | <= | =>) expr)?

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
            new (MemoryLinearAllocate(&astBuffer, sizeof(ASTRelOp), 8))
            ASTRelOp(op, node, expr());
    }

    return node;
}

ASTNode* Parser::cond_equal()
{
    // cond_equal : cond_expr ((== | !=) cond_expr)?
    // cond_equal : LPAREN cond_or RPAREN
    // cond_equal : NOT LPAREN cond_or RPAREN
    // cond_equal : NOT factor

    ASTNode* node = nullptr;

    if (currentToken.type == TokenType::LParen)
    {
        eat(TokenType::LParen);
        node = cond_or();
        eat(TokenType::RParen);
        return node;
    }
    else if (currentToken.type == TokenType::LogicalNegation)
    {
        eat(TokenType::LogicalNegation);
        if(currentToken.type == TokenType::LParen)
        {
            eat(TokenType::LParen);
            node =
                new (MemoryLinearAllocate(&astBuffer, sizeof(ASTLogicalNot), 8))
                ASTLogicalNot(cond_or());
            eat(TokenType::RParen);
        }
        else
        {
            node =
                new (MemoryLinearAllocate(&astBuffer, sizeof(ASTLogicalNot), 8))
                ASTLogicalNot(factor());
        }
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
            new (MemoryLinearAllocate(&astBuffer, sizeof(ASTRelOp), 8))
            ASTRelOp(op, node, cond_expr());
    }

    return node;
}

ASTNode* Parser::cond_and()
{
    // cond_and : cond_equal (AND cond_equal)?

    auto node = cond_equal();

    if(currentToken.type == TokenType::LogicalAnd)
    {
        eat(TokenType::LogicalAnd);

        node =
            new (MemoryLinearAllocate(&astBuffer, sizeof(ASTRelOp), 8))
            ASTRelOp(RelOp::AND, node, cond_equal());
    }

    return node;
}

ASTNode* Parser::cond_or()
{
    // cond_or : cond_and (OR cond_and)?

    auto node = cond_and();

    if(currentToken.type == TokenType::LogicalOr)
    {
        eat(TokenType::LogicalOr);

        node =
            new (MemoryLinearAllocate(&astBuffer, sizeof(ASTRelOp), 8))
            ASTRelOp(RelOp::OR, node, cond_and());
    }

    return node;
}

ASTNode* Parser::factor()
{
    // factor : NUMBER
    // factor : LPAREN expr RPAREN
    // factor : VARIABLEIDENTIFIER
    // factor : TRUE|FALSE
    // todo : function calls

    auto t = currentToken;
    ASTNode* node = nullptr;

    if (t.type == TokenType::NumberLiteral)
    {
        eat(TokenType::NumberLiteral);
        node =
            new(MemoryLinearAllocate(&astBuffer, sizeof(ASTNumberTerminal), 8)) 
            ASTNumberTerminal(atoi(t.text.c_str()));
        return node;
    }
    else if (t.type == TokenType::True)
    {
        eat(TokenType::True);
        node = 
            new(MemoryLinearAllocate(&astBuffer, sizeof(ASTBooleanTerminal), 8)) 
            ASTBooleanTerminal(true);
    }
    else if (t.type == TokenType::False)
    {
        eat(TokenType::False);
        node = 
            new(MemoryLinearAllocate(&astBuffer, sizeof(ASTBooleanTerminal), 8)) 
            ASTBooleanTerminal(false);
    }
    else if (t.type == TokenType::LParen)
    {
        eat(TokenType::LParen);
        node = expr();
        eat(TokenType::RParen);
    }
    else if (t.type == TokenType::Identifier)
    {
        eat(TokenType::Identifier);
        node = 
            new (MemoryLinearAllocate(&astBuffer, sizeof(ASTVariable), 32))
            ASTVariable(t.text);
    }
    else
    {
        error();
    }

    return node;
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



struct TValue
{
    enum class ValueType
    {
        Invalid,
        Integer,
        Real,
        Boolean
    };

    union
    {
        i64 integerValue;
        float realValue;
        bool boolValue;
    };

    ValueType type = ValueType::Invalid;
};



i8 printAstIndent = 0;
void PrintAST(ASTNode* ast)
{
    printAstIndent += 3;
    if(ast == nullptr)
    {
        printf("%s\n", (std::string(printAstIndent, ' ') + std::string("null")).c_str());
        printAstIndent -= 3;
        return;
    }
    switch(ast->GetType())
    {
        case ASTNodeType::ASSIGN:  {
            auto v = static_cast<ASTAssignment*>(ast);
            printf("%s\n", (std::string(printAstIndent, ' ') + std::string("assign ")).c_str());
            PrintAST(v->id);
            PrintAST(v->expr);
        } break;
        case ASTNodeType::RETURN: {
            auto v = static_cast<ASTReturn*>(ast);
            printf("%s\n", (std::string(printAstIndent, ' ') + std::string("return ")).c_str());
            PrintAST(v->expr);
        } break;
        case ASTNodeType::BINOP: {
            auto v = static_cast<ASTBinOp*>(ast);
            const char* opName = nullptr;
            switch (v->op)
            {
                case BinOp::Add: opName = "add"; break;
                case BinOp::Sub: opName = "sub"; break;
                case BinOp::Mul: opName = "mul"; break;
                case BinOp::Div: opName = "div"; break;
            }
            printf("%s%s%s\n", (std::string(printAstIndent, ' ') + std::string("binop l ")).c_str(), opName, " r");
            PrintAST(v->left);
            PrintAST(v->right);
        } break;
        case ASTNodeType::RELOP: {
            auto v = static_cast<ASTRelOp*>(ast);
            const char* opName = nullptr;
            switch (v->op)
            {
                case RelOp::LT: opName = "less than"; break;
                case RelOp::GT: opName = "greater than"; break;
                case RelOp::LE: opName = "less than or equal"; break;
                case RelOp::GE: opName = "greater than or equal"; break;
                case RelOp::EQ: opName = "equal"; break;
                case RelOp::NEQ: opName = "not equal"; break;
                case RelOp::AND: opName = "and"; break;
                case RelOp::OR: opName = "or"; break;
            }
            printf("%s%s%s\n", (std::string(printAstIndent, ' ') + std::string("relop l ")).c_str(), opName, " r");
            PrintAST(v->left);
            PrintAST(v->right);
        } break;
        case ASTNodeType::LOGICALNOT: {
            auto v = static_cast<ASTLogicalNot*>(ast);
            printf("%s\n", (std::string(printAstIndent, ' ') + std::string("not")).c_str());
            PrintAST(v->boolExpr);
        } break;
        case ASTNodeType::BRANCH: {
            auto v = static_cast<ASTBranch*>(ast);
            printf("%s\n", (std::string(printAstIndent, ' ') + std::string("branch (cond, if, else)")).c_str());
            PrintAST(v->condition);
            PrintAST(v->if_body);
            PrintAST(v->else_body);
        } break;
        case ASTNodeType::VARIABLE: {
            auto v = static_cast<ASTVariable*>(ast);
            printf("%s\n", (std::string(printAstIndent, ' ') + std::string("var ") + v->id).c_str());
        } break;
        case ASTNodeType::NUMBER: {
            auto v = static_cast<ASTNumberTerminal*>(ast);
            printf("%s%d\n", (std::string(printAstIndent, ' ') + std::string("num ")).c_str(), v->value);
        } break;
        case ASTNodeType::BOOLEAN: {
            auto v = static_cast<ASTBooleanTerminal*>(ast);
            printf("%s%s\n", (std::string(printAstIndent, ' ') + std::string("bool ")).c_str(), (v->value ? "true" : "false"));
        } break;
    }
    printAstIndent -= 3;
}

TValue InterpretExpression(ASTNode* ast)
{
    switch(ast->GetType())
    {
        case ASTNodeType::BINOP: {
            auto v = static_cast<ASTBinOp*>(ast);
            TValue l = InterpretExpression(v->left);
            TValue r = InterpretExpression(v->right);
            // both integer, then integer
            // both float, then float
            // one int, one float, then float
            TValue::ValueType retValType = TValue::ValueType::Integer;
            switch (v->op)
            {
                case BinOp::Add: {
                    if (l.type == TValue::ValueType::Integer && r.type == TValue::ValueType::Integer)
                    {
                        return { .integerValue=l.integerValue + r.integerValue, .type=TValue::ValueType::Integer };
                    }
                    else if (l.type == TValue::ValueType::Real && r.type == TValue::ValueType::Real)
                    {
                        return { .realValue=l.realValue + r.integerValue, .type=TValue::ValueType::Real };
                    }
                    else if (l.type == TValue::ValueType::Integer && r.type == TValue::ValueType::Real)
                    {
                        return { .realValue=l.integerValue + r.realValue, .type=TValue::ValueType::Real };
                    }
                    else if (l.type == TValue::ValueType::Real && r.type == TValue::ValueType::Integer)
                    {
                        return { .realValue=l.realValue + r.realValue, .type=TValue::ValueType::Real };
                    }
                    else
                    {
                        // todo error
                    }
                } break;
                case BinOp::Sub: {
                    if (l.type == TValue::ValueType::Integer && r.type == TValue::ValueType::Integer)
                    {
                        return { .integerValue=l.integerValue - r.integerValue, .type=TValue::ValueType::Integer };
                    }
                    else if (l.type == TValue::ValueType::Real && r.type == TValue::ValueType::Real)
                    {
                        return { .realValue=l.realValue - r.integerValue, .type=TValue::ValueType::Real };
                    }
                    else if (l.type == TValue::ValueType::Integer && r.type == TValue::ValueType::Real)
                    {
                        return { .realValue=l.integerValue - r.realValue, .type=TValue::ValueType::Real };
                    }
                    else if (l.type == TValue::ValueType::Real && r.type == TValue::ValueType::Integer)
                    {
                        return { .realValue=l.realValue - r.realValue, .type=TValue::ValueType::Real };
                    }
                    else
                    {
                        // todo error
                    }
                } break;
                case BinOp::Mul: {
                    if (l.type == TValue::ValueType::Integer && r.type == TValue::ValueType::Integer)
                    {
                        return { .integerValue=l.integerValue * r.integerValue, .type=TValue::ValueType::Integer };
                    }
                    else if (l.type == TValue::ValueType::Real && r.type == TValue::ValueType::Real)
                    {
                        return { .realValue=l.realValue * r.integerValue, .type=TValue::ValueType::Real };
                    }
                    else if (l.type == TValue::ValueType::Integer && r.type == TValue::ValueType::Real)
                    {
                        return { .realValue=l.integerValue * r.realValue, .type=TValue::ValueType::Real };
                    }
                    else if (l.type == TValue::ValueType::Real && r.type == TValue::ValueType::Integer)
                    {
                        return { .realValue=l.realValue * r.realValue, .type=TValue::ValueType::Real };
                    }
                    else
                    {
                        // todo error
                    }
                } break;
                case BinOp::Div: {
                    if (l.type == TValue::ValueType::Integer && r.type == TValue::ValueType::Integer)
                    {
                        return { .integerValue=l.integerValue / r.integerValue, .type=TValue::ValueType::Integer };
                    }
                    else if (l.type == TValue::ValueType::Real && r.type == TValue::ValueType::Real)
                    {
                        return { .realValue=l.realValue / r.integerValue, .type=TValue::ValueType::Real };
                    }
                    else if (l.type == TValue::ValueType::Integer && r.type == TValue::ValueType::Real)
                    {
                        return { .realValue=l.integerValue / r.realValue, .type=TValue::ValueType::Real };
                    }
                    else if (l.type == TValue::ValueType::Real && r.type == TValue::ValueType::Integer)
                    {
                        return { .realValue=l.realValue / r.realValue, .type=TValue::ValueType::Real };
                    }
                    else
                    {
                        // todo error
                    }
                } break;
            }
        } break;
        case ASTNodeType::RELOP: {
            auto v = static_cast<ASTRelOp*>(ast);
            TValue l = InterpretExpression(v->left);
            TValue r = InterpretExpression(v->right);
            if (l.type == TValue::ValueType::Integer)
            {
                l.realValue = float(l.integerValue);
                l.type = TValue::ValueType::Real;
            }
            else if (l.type == TValue::ValueType::Boolean)
            {
                l.realValue = l.boolValue ? 1.f : 0.f;
                l.type = TValue::ValueType::Real;
            }
            if (r.type == TValue::ValueType::Integer)
            {
                r.realValue = float(r.integerValue);
                r.type = TValue::ValueType::Real;
            }
            else if (r.type == TValue::ValueType::Boolean)
            {
                r.realValue = l.boolValue ? 1.f : 0.f;
                r.type = TValue::ValueType::Real;
            }
            switch (v->op)
            {
                case RelOp::LT: return { .boolValue=l.realValue < r.realValue, .type=TValue::ValueType::Boolean };
                case RelOp::GT: return { .boolValue=l.realValue > r.realValue, .type=TValue::ValueType::Boolean };
                case RelOp::LE: return { .boolValue=l.realValue <= r.realValue, .type=TValue::ValueType::Boolean };
                case RelOp::GE: return { .boolValue=l.realValue >= r.realValue, .type=TValue::ValueType::Boolean };
                case RelOp::EQ: return { .boolValue=l.realValue == r.realValue, .type=TValue::ValueType::Boolean };
                case RelOp::NEQ: return { .boolValue=l.realValue != r.realValue, .type=TValue::ValueType::Boolean };
                case RelOp::AND: return { .boolValue=(l.realValue == 1.f && r.realValue == 1.f), .type=TValue::ValueType::Boolean };
                case RelOp::OR: return { .boolValue=(l.realValue == 1.f || r.realValue == 1.f), .type=TValue::ValueType::Boolean };
            }
        } break;
        case ASTNodeType::LOGICALNOT: {
            auto v = static_cast<ASTLogicalNot*>(ast);
            auto result = InterpretExpression(v->boolExpr);
            result.boolValue = !result.boolValue;
            return result;
        } break;
//        case ASTNodeType::VARIABLE: {
//            auto v = static_cast<ASTVariable*>(ast);
//            printf("%s\n", (std::string(printAstIndent, ' ') + std::string("var ") + v->id).c_str());
//        } break;
        case ASTNodeType::NUMBER: {
            auto v = static_cast<ASTNumberTerminal*>(ast);
            return { .integerValue=v->value, .type=TValue::ValueType::Integer };
        } break;
        case ASTNodeType::BOOLEAN: {
            auto v = static_cast<ASTBooleanTerminal*>(ast);
            return { .boolValue = v->value, .type=TValue::ValueType::Boolean };
        } break;
    }
}

void InterpretStatement(ASTNode* statement)
{
    switch(statement->GetType())
    {
        case ASTNodeType::ASSIGN:  {
            auto v = static_cast<ASTAssignment*>(statement);
            // assign v->expr to v->id
            auto result = InterpretExpression(v->expr);
            // assign result to v->id
            // todo

        } break;
        case ASTNodeType::RETURN: {
            auto v = static_cast<ASTReturn*>(statement);
            TValue result = InterpretExpression(v->expr);
            printf("result %lld\n", result.integerValue);
        } break;
        case ASTNodeType::BRANCH: {
            auto v = static_cast<ASTBranch*>(statement);
            auto condition = InterpretExpression(v->condition);
            ASSERT(condition.type == TValue::ValueType::Boolean);
            if(condition.boolValue)
                InterpretStatement(v->if_body);
            else
                InterpretStatement(v->else_body);
        } break;
    }
}


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
    auto result = Lexer(" "
                        "if (4 < 32) "
                        "   if (false) "
                        "       return 3 "
                        "   else "
                        "       return 7 "
                        "else "
                        "   return -2 - 4 ");
    auto parser = Parser(result);
    auto v = parser.parse();
    for (auto n : v)
    {
        InterpretStatement(n);
        //PrintAST(n);
    }

//    printf("hello\n");
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
