#include "Compiler.h"

#include "Chunk.h"
#include "Scanner.h"
#ifdef DEBUG_PRINT_CODE
#include "Debug.h"
#endif
#include "Object.h"

typedef void(*ParseFn)();

enum class Precedence : u8
{
    NONE,
    ASSIGNMENT, // =
    OR,         // or
    AND,        // and
    EQUALITY,   // == !=
    COMPARISON, // < > <= >=
    TERM,       // + -
    FACTOR,     // * /
    UNARY,      // ! -
    CALL
};

struct ParseRule
{
    ParseFn     prefixfn;
    ParseFn     infixfn;
    Precedence  infixprecedence;
};

static void ParsePrecedence(Precedence precedence);
static ParseRule *GetParseRule(TokenType type);

struct Parser
{
    Token current;
    Token previous;

    bool hadError = false;
    bool panicMode = false;
};

Parser parser;


static void ErrorAt(Token *token, const char *message) 
{
    if (parser.panicMode) return;
    parser.panicMode = true;

    fprintf(stderr, "[line %d] Compile error", token->line);

    if (token->type == TokenType::END_OF_FILE)
    {
        fprintf(stderr, " at end");
    }
    else if (token->type == TokenType::ERROR) 
    {
        // Nothing.
    }
    else 
    {
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }

    fprintf(stderr, ": %s\n", message);
    parser.hadError = true;
}

static void ErrorAtCurrent(const char *message) 
{
    ErrorAt(&parser.current, message);
}

static void Error(const char *message) 
{
    ErrorAt(&parser.previous, message);
}

static void Advance()
{
    parser.previous = parser.current;

    for (;;)
    {
        parser.current = ScanToken();
        if (parser.current.type != TokenType::ERROR) break;
        ErrorAtCurrent(parser.current.start);
    }
}

static void Eat(TokenType type, const char *errorMsg)
{
    if (parser.current.type == type)
    {
        Advance();
        return;
    }

    ErrorAtCurrent(errorMsg);
}

Chunk *compilingChunk;

static Chunk *CurrentChunk()
{
    return compilingChunk;
}

static void EmitByte(u8 byte)
{
    WriteChunk(CurrentChunk(), byte, parser.previous.line);
}

static void EmitByte(OpCode op)
{
    EmitByte((u8)op);
}

static void EmitConstant(TValue value)
{
    WriteConstant(CurrentChunk(), value, parser.previous.line);
}

static void EmitBytes(u8 byte1, u8 byte2)
{
    EmitByte(byte1);
    EmitByte(byte2);
}

static void EmitReturn()
{
    EmitByte(OpCode::RETURN);
}

static void EndCompiler()
{
    EmitReturn();
#ifdef DEBUG_PRINT_CODE
    if (!parser.hadError)
    {
        DisassembleChunk(CurrentChunk(), "Compiled code disassembly");
    }
#endif
}


static void ParseNumber()
{
    double value = strtod(parser.previous.start, NULL);
    EmitConstant(NUMBER_VAL(value));
}

static void ParseString()
{
    EmitConstant(RCOBJ_VAL((RCObject*)CopyString(parser.previous.start + 1, parser.previous.length - 2)));
}

static void ParseExpression()
{
    ParsePrecedence(Precedence::ASSIGNMENT);
}

static void ParseGrouping()
{
    ParseExpression();
    Eat(TokenType::RPAREN, "Expected ')' after expression.");
}

static void ParseUnary()
{
    TokenType operatorType = parser.previous.type;

    ParsePrecedence(Precedence::UNARY); // Compile the operand

    switch (operatorType)
    {
        case TokenType::BANG:  EmitByte(OpCode::LOGICAL_NOT); break;
        case TokenType::MINUS: EmitByte(OpCode::NEGATE); break;
    }
}

static void ParseBinOp()
{
    TokenType operatorType = parser.previous.type;
    ParseRule *rule = GetParseRule(operatorType);
    ParsePrecedence(Precedence((u8)rule->infixprecedence + 1));

    switch (operatorType)
    {
        case TokenType::PLUS:           EmitByte(OpCode::ADD); break;
        case TokenType::MINUS:          EmitByte(OpCode::SUBTRACT); break;
        case TokenType::ASTERISK:       EmitByte(OpCode::MULTIPLY); break;
        case TokenType::FORWARDSLASH:   EmitByte(OpCode::DIVIDE); break;
    }
}

static void ParseRelOp()
{
    TokenType operatorType = parser.previous.type;
    ParsePrecedence((Precedence)((u8)GetParseRule(operatorType)->infixprecedence + 1));

    switch (operatorType)
    {
        case TokenType::BANG_EQUAL:
            EmitByte(OpCode::RELOP_EQUAL);
            EmitByte(OpCode::LOGICAL_NOT);
            break;
        case TokenType::EQUAL_EQUAL:
            EmitByte(OpCode::RELOP_EQUAL);
            break;
        case TokenType::GREATER:
            EmitByte(OpCode::RELOP_GREATER);
            break;
        case TokenType::LESS:
            EmitByte(OpCode::RELOP_LESSER);
            break;
        case TokenType::GREATER_EQUAL:
            EmitByte(OpCode::RELOP_LESSER);
            EmitByte(OpCode::LOGICAL_NOT);
            break;
        case TokenType::LESS_EQUAL:
            EmitByte(OpCode::RELOP_GREATER);
            EmitByte(OpCode::LOGICAL_NOT);
            break;
    }
}

static void ParseLiteral()
{
    switch (parser.previous.type) 
    {
        case TokenType::TRUE: EmitByte((u8)OpCode::OP_TRUE); break;
        case TokenType::FALSE: EmitByte((u8)OpCode::OP_FALSE); break;
    }
}

ParseRule rules[(u8)TokenType::END_OF_FILE];

void SetupParsingRules()
{
    rules[(u8)TokenType::LESS]              = {          NULL,   ParseRelOp, Precedence::COMPARISON };
    rules[(u8)TokenType::LESS_EQUAL]        = {          NULL,   ParseRelOp, Precedence::COMPARISON };
    rules[(u8)TokenType::GREATER]           = {          NULL,   ParseRelOp, Precedence::COMPARISON };
    rules[(u8)TokenType::GREATER_EQUAL]     = {          NULL,   ParseRelOp, Precedence::COMPARISON };
    rules[(u8)TokenType::EQUAL]             = {          NULL,         NULL, Precedence::NONE };
    rules[(u8)TokenType::BANG_EQUAL]        = {          NULL,   ParseRelOp, Precedence::EQUALITY };
    rules[(u8)TokenType::EQUAL_EQUAL]       = {          NULL,   ParseRelOp, Precedence::EQUALITY };
    rules[(u8)TokenType::BANG]              = {    ParseUnary,         NULL, Precedence::NONE };
    rules[(u8)TokenType::LSQBRACK]          = {          NULL,         NULL, Precedence::NONE };
    rules[(u8)TokenType::RSQBRACK]          = {          NULL,         NULL, Precedence::NONE };
    rules[(u8)TokenType::LPAREN]            = { ParseGrouping,         NULL, Precedence::NONE };
    rules[(u8)TokenType::RPAREN]            = {          NULL,         NULL, Precedence::NONE };
    rules[(u8)TokenType::LBRACE]            = {          NULL,         NULL, Precedence::NONE };
    rules[(u8)TokenType::RBRACE]            = {          NULL,         NULL, Precedence::NONE };
    rules[(u8)TokenType::COMMA]             = {          NULL,         NULL, Precedence::NONE };
    rules[(u8)TokenType::PLUS]              = {          NULL,   ParseBinOp, Precedence::TERM };
    rules[(u8)TokenType::MINUS]             = {    ParseUnary,   ParseBinOp, Precedence::TERM };
    rules[(u8)TokenType::ASTERISK]          = {          NULL,   ParseBinOp, Precedence::FACTOR };
    rules[(u8)TokenType::FORWARDSLASH]      = {          NULL,   ParseBinOp, Precedence::FACTOR };
    
    rules[(u8)TokenType::NUMBER_LITERAL]    = {   ParseNumber,         NULL, Precedence::NONE };
    rules[(u8)TokenType::STRING_LITERAL]    = {   ParseString,         NULL, Precedence::NONE };
    rules[(u8)TokenType::IDENTIFIER]        = {          NULL,         NULL, Precedence::NONE };
    
    rules[(u8)TokenType::TRUE]              = {  ParseLiteral,         NULL, Precedence::NONE };
    rules[(u8)TokenType::FALSE]             = {  ParseLiteral,         NULL, Precedence::NONE };
    rules[(u8)TokenType::AND]               = {          NULL,         NULL, Precedence::NONE };
    rules[(u8)TokenType::OR]                = {          NULL,         NULL, Precedence::NONE };
    rules[(u8)TokenType::FN]                = {          NULL,         NULL, Precedence::NONE };
    rules[(u8)TokenType::IF]                = {          NULL,         NULL, Precedence::NONE };
    rules[(u8)TokenType::ELSE]              = {          NULL,         NULL, Precedence::NONE };
    rules[(u8)TokenType::WHILE]             = {          NULL,         NULL, Precedence::NONE };
    rules[(u8)TokenType::RETURN]            = {          NULL,         NULL, Precedence::NONE };
    
    rules[(u8)TokenType::ERROR]             = {          NULL,         NULL, Precedence::NONE };
};

static void ParsePrecedence(Precedence precedence)
{
    Advance();
    ParseFn prefixfn = GetParseRule(parser.previous.type)->prefixfn;
    if (prefixfn == NULL)
    {
        Error("Expected expression.");
        return;
    }
    prefixfn();

    while (precedence <= GetParseRule(parser.current.type)->infixprecedence)
    {
        Advance();
        ParseFn infixfn = GetParseRule(parser.previous.type)->infixfn;
        infixfn();
    }
}

static ParseRule *GetParseRule(TokenType type)
{
    return &rules[(u8)type];
}


bool Compile(const char *source, Chunk *chunk)
{
    SetupParsingRules();

    InitScanner(source);
    compilingChunk = chunk;

    parser.hadError = false;
    parser.panicMode = false;

    Advance();
    ParseExpression();
    Eat(TokenType::END_OF_FILE, "Expected end of expression.");

    EndCompiler();
    return !parser.hadError;
}
