#include "Compiler.h"

#include "Chunk.h"
#include "Scanner.h"

static void Advance()
{
    
}

static void Expression()
{

}

static void Consume(TokenType type, const char *errorMsg)
{

}

bool Compile(const char *source, Chunk *chunk)
{
    InitScanner(source);

    Advance();
    Expression();
    Consume(TokenType::END_OF_FILE, "Expected end of expression.");
}
