#pragma once

#include "../MesaCommon.h"

enum class TokenType : u8
{
    LESS,
    LESS_EQUAL,
    GREATER,
    GREATER_EQUAL,
    EQUAL,
    BANG_EQUAL,
    EQUAL_EQUAL,
    BANG,
    LSQBRACK,
    RSQBRACK,
    LPAREN,
    RPAREN,
    LBRACE,
    RBRACE,
    COMMA,
    DOT,
    PLUS,
    MINUS,
    ASTERISK,
    FORWARDSLASH,

    NUMBER_LITERAL,
    STRING_LITERAL,
    IDENTIFIER,

    TRUE,
    FALSE,
    AND,
    OR,
    FN,
    IF,
    ELSE,
    WHILE,
    FOR,
    MUT,
    RETURN,
    PRINT,

    ERROR,
    END_OF_FILE
};

struct Token
{
    TokenType type;
    const char *start;
    int length;
    int line;
};

struct Scanner
{
    const char *start;
    const char *current;
    int line;
};

void InitScanner(const char *source);
Token ScanToken();
void Debug_ScanPrintTokens(const char *source);

