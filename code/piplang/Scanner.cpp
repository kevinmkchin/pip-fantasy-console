#include "Scanner.h"

#include <stdio.h>
#include <string>

Scanner scanner;

void InitScanner(const char *source)
{
    scanner.start = source;
    scanner.current = scanner.start;
    scanner.line = 1;
}

static Token MakeToken(TokenType type)
{
    Token token;
    token.type = type;
    token.start = scanner.start;
    token.length = (int)(scanner.current - scanner.start);
    token.line = scanner.line;
    return token;
}

static Token ErrorToken(const char *msg)
{
    Token token;
    token.type = TokenType::ERROR;
    token.start = msg;
    token.length = (int)strlen(msg);
    token.line = scanner.line;
    return token;
}

static char Advance()
{
    ++scanner.current;
    return scanner.current[-1];
}

static bool Match(char expected)
{
    if (*scanner.current == '\0') return false;
    if (*scanner.current != expected) return false;
    ++scanner.current;
    return true;
}

static char PeekNext()
{
    if (*scanner.current == '\0') return '\0';
    return *(scanner.current + 1);
}

static void SkipWhiteSpace()
{
    for (;;)
    {
        char c = *scanner.current;
        switch (c)
        {
        case ';': // Line comments
        {
            while (*scanner.current != '\n' && *scanner.current != '\0')
                Advance();
            break;
        }
        case '/':
        {
            if (PeekNext() == '*')
            {
                Advance();
                Advance();
                while (!(*scanner.current == '*' && PeekNext() == '/') && *scanner.current != '\0')
                    Advance();
                Advance();
                Advance();
            }
            else
            {
                return;
            }
            break;
        }

        case ' ':
        case '\r':
        case '\t':
            Advance();
            break;
        case '\n':
            ++scanner.line;
            Advance();
            break;
        default:
            return;
        }
    }
}

static Token StringToken(bool doubleQuote)
{
    char endQuote = doubleQuote ? '"' : '\'';

    while (*scanner.current != endQuote && *scanner.current != '\0')
    {
        if (*scanner.current == '\n')
            ++scanner.line;
        Advance();
    }

    if (*scanner.current == '\0')
        return ErrorToken("Unterminated string.");

    Advance();
    return MakeToken(TokenType::STRING_LITERAL);
}

static bool IsDigit(char c)
{
    return ('0' <= c && c <= '9');
}

static Token NumberToken()
{
    while (IsDigit(*scanner.current))
        Advance();

    if (*scanner.current == '.' && IsDigit(PeekNext()))
    {
        Advance();
        while (IsDigit(*scanner.current))
            Advance();
    }

    return MakeToken(TokenType::NUMBER_LITERAL);
}

static bool IsNamingAlphabet(char c)
{
    return ('a' <= c && c <= 'z')
        || ('A' <= c && c <= 'Z')
        || ('_' == c);
}

static TokenType IdentifierType()
{
    std::string word = std::string(scanner.start, scanner.current - scanner.start);

    if (word == "true") return TokenType::TRUE;
    if (word == "false") return TokenType::FALSE;
    if (word == "and") return TokenType::AND;
    if (word == "or") return TokenType::OR;
    if (word == "if") return TokenType::IF;
    if (word == "else") return TokenType::ELSE;
    if (word == "while") return TokenType::WHILE;
    if (word == "for") return TokenType::FOR;
    if (word == "fn") return TokenType::FN;
    if (word == "mut") return TokenType::MUT;
    if (word == "return") return TokenType::RETURN;
    if (word == "print") return TokenType::PRINT;

    return TokenType::IDENTIFIER;
}

static Token IdentifierToken()
{
    while (IsNamingAlphabet(*scanner.current) || IsDigit(*scanner.current))
        Advance();
    return MakeToken(IdentifierType());
}

Token ScanToken()
{
    SkipWhiteSpace();
    scanner.start = scanner.current;

    if (*scanner.current == '\0') return MakeToken(TokenType::END_OF_FILE);

    char c = Advance();

    if (IsDigit(c)) return NumberToken();
    if (IsNamingAlphabet(c)) return IdentifierToken();
    switch (c)
    {
        case '(': return MakeToken(TokenType::LPAREN);
        case ')': return MakeToken(TokenType::RPAREN);
        case '{': return MakeToken(TokenType::LBRACE);
        case '}': return MakeToken(TokenType::RBRACE);
        case '[': return MakeToken(TokenType::LSQBRACK);
        case ']': return MakeToken(TokenType::RSQBRACK);
        case ',': return MakeToken(TokenType::COMMA);
        case '.': return MakeToken(TokenType::DOT);
        case '-': return MakeToken(TokenType::MINUS);
        case '+': return MakeToken(TokenType::PLUS);
        case '/': return MakeToken(TokenType::FORWARDSLASH);
        case '*': return MakeToken(TokenType::ASTERISK);
        case ':': return MakeToken(TokenType::COLON);
        case '!':
            return MakeToken(Match('=') ? TokenType::BANG_EQUAL : TokenType::BANG);
        case '=':
            return MakeToken(Match('=') ? TokenType::EQUAL_EQUAL : TokenType::EQUAL);
        case '<':
            return MakeToken(Match('=') ? TokenType::LESS_EQUAL : TokenType::LESS);
        case '>':
            return MakeToken(Match('=') ? TokenType::GREATER_EQUAL : TokenType::GREATER);
        case '"': return StringToken(true);
        case '\'': return StringToken(false);
    }

    return ErrorToken("Scanned unexpected character.");
}

void Debug_ScanPrintTokens(const char *source)
{
    int line = -1;
    for (;;)
    {
        Token token = ScanToken();

        if (token.line != line)
        {
            printf("%4d ", token.line);
            line = token.line;
        }
        else
        {
            printf("   | ");
        }

        printf("%2d '%.*s'\n", token.type, token.length, token.start);

        if (token.type == TokenType::END_OF_FILE) break;
    }
}
