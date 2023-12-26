#include "Compiler.h"

#include "Chunk.h"
#include "Scanner.h"
#ifdef DEBUG_PRINT_CODE
#include "Debug.h"
#endif

typedef void(*ParseFn)();

enum class Precedence : u8
{
    NONE,
    ASSIGNMENT, // =
    OR,         // or
    AND,        // and
    EQUALITY,   // == !=
    RELOP,      // < > <= >=
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

    fprintf(stderr, "[line %d] Error", token->line);

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
    EmitByte((u8)OpCode::RETURN);
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
    TValue v;
    v.real = value;
    EmitConstant(v);
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
        case TokenType::MINUS: EmitByte((u8)OpCode::NEGATE); break;
    }
}

static void ParseBinOp()
{
    TokenType operatorType = parser.previous.type;
    ParseRule *rule = GetParseRule(operatorType);
    ParsePrecedence(Precedence((u8)rule->infixprecedence + 1));

    switch (operatorType)
    {
        case TokenType::PLUS:           EmitByte((u8)OpCode::ADD); break;
        case TokenType::MINUS:          EmitByte((u8)OpCode::SUBTRACT); break;
        case TokenType::ASTERISK:       EmitByte((u8)OpCode::MULTIPLY); break;
        case TokenType::FORWARDSLASH:   EmitByte((u8)OpCode::DIVIDE); break;
    }
}


ParseRule rules[(u8)TokenType::END_OF_FILE];

void SetupParsingRules()
{
    rules[(u8)TokenType::LESS]              = {          NULL,         NULL, Precedence::NONE };
    rules[(u8)TokenType::LESS_EQUAL]        = {          NULL,         NULL, Precedence::NONE };        
    rules[(u8)TokenType::GREATER]           = {          NULL,         NULL, Precedence::NONE };    
    rules[(u8)TokenType::GREATER_EQUAL]     = {          NULL,         NULL, Precedence::NONE };        
    rules[(u8)TokenType::EQUAL]             = {          NULL,         NULL, Precedence::NONE };
    rules[(u8)TokenType::BANG_EQUAL]        = {          NULL,         NULL, Precedence::NONE };        
    rules[(u8)TokenType::EQUAL_EQUAL]       = {          NULL,         NULL, Precedence::NONE };        
    rules[(u8)TokenType::BANG]              = {          NULL,         NULL, Precedence::NONE };
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
    rules[(u8)TokenType::STRING_LITERAL]    = {          NULL,         NULL, Precedence::NONE };            
    rules[(u8)TokenType::IDENTIFIER]        = {          NULL,         NULL, Precedence::NONE }; 
    
    rules[(u8)TokenType::TRUE]              = {          NULL,         NULL, Precedence::NONE };
    rules[(u8)TokenType::FALSE]             = {          NULL,         NULL, Precedence::NONE };
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
