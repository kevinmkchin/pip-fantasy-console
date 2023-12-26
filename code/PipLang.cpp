#include "PipLang.h"

#include <string>
#include <vector>
#include <stdio.h>


enum class OpCode : u8
{
    RETURN,
    CONSTANT,
    CONSTANT_LONG,
    NEGATE,
    ADD,
    SUBTRACT,
    MULTIPLY,
    DIVIDE
};

struct TValue
{
    union
    {
        double real;
    };
};

struct Chunk
{
    std::vector<int> *linenumbers; // TODO(Kevin): less memory hogging line number encoding
    std::vector<u8> *bytecode;
    std::vector<TValue> *constants;
};


void InitChunk(Chunk *chunk)
{
    chunk->linenumbers = new std::vector<int>();
    chunk->bytecode = new std::vector<u8>(); // instead of dynamic array like so, I could use my linear memory allocator for all static bytecode at compile time.
    chunk->constants = new std::vector<TValue>();
}

void FreeChunk(Chunk *chunk)
{
    chunk->linenumbers->clear();
    chunk->bytecode->clear();
    chunk->constants->clear();
    // Note(Kevin): I'm choosing not to delete the std::vectors. Keep them alive but just empty.
}

void WriteChunk(Chunk *chunk, u8 byte, int line)
{
    chunk->linenumbers->push_back(line);
    chunk->bytecode->push_back(byte);
}

void WriteConstant(Chunk *chunk, TValue value, int line)
{
    chunk->constants->push_back(value);
    u32 cindex = (u32)chunk->constants->size() - 1;
    if (cindex >= 256)
    {
        WriteChunk(chunk, (u8)OpCode::CONSTANT_LONG, line);
        WriteChunk(chunk, (u8)(cindex >> 16), line);
        WriteChunk(chunk, (u8)(cindex >> 8), line);
        WriteChunk(chunk, (u8)cindex, line);
    }
    else
    {
        WriteChunk(chunk, (u8)OpCode::CONSTANT, line);
        WriteChunk(chunk, (u8)cindex, line);
    }
}

void PrintTValue(TValue value)
{
    printf("%g", value.real);
}

int Debug_SimpleInstruction(const char *name, int offset)
{
    printf("%s\n", name);
    return offset + 1;
}

int Debug_ConstantInstruction(const char *name, Chunk *chunk, int offset)
{
    u8 constantIndex = chunk->bytecode->at(offset + 1);
    printf("%-16s %4d '", name, constantIndex);
    PrintTValue(chunk->constants->at(constantIndex));
    printf("'\n");
    return offset + 2;
}

int Debug_ConstantLongInstruction(const char *name, Chunk *chunk, int offset)
{
    u32 byte2 = chunk->bytecode->at(offset + 1);
    u32 byte1 = chunk->bytecode->at(offset + 2);
    u32 byte0 = chunk->bytecode->at(offset + 3);
    u32 constantIndex = byte2 << 16 | byte1 << 8 | byte0;
    printf("%-16s %4d '", name, constantIndex);
    PrintTValue(chunk->constants->at(constantIndex));
    printf("'\n");
    return offset + 4;
}

int Debug_DisassembleInstruction(Chunk *chunk, int offset)
{
    printf("%04d ", offset);

    if (offset > 0 && chunk->linenumbers->at(offset) == chunk->linenumbers->at(offset - 1))
    {
        printf("   | ");
    }
    else
    {
        printf("%4d ", chunk->linenumbers->at(offset));
    }

    OpCode instruction = (OpCode)chunk->bytecode->at(offset);
    switch (instruction)
    {
    case OpCode::RETURN:
        return Debug_SimpleInstruction("RETURN", offset);
    case OpCode::CONSTANT:
        return Debug_ConstantInstruction("CONSTANT", chunk, offset);
    case OpCode::CONSTANT_LONG:
        return Debug_ConstantLongInstruction("CONSTANT_LONG", chunk, offset);
    case OpCode::NEGATE:
        return Debug_SimpleInstruction("NEGATE", offset);
    case OpCode::ADD:
        return Debug_SimpleInstruction("ADD", offset);
    case OpCode::SUBTRACT:
        return Debug_SimpleInstruction("SUBTRACT", offset);
    case OpCode::MULTIPLY:
        return Debug_SimpleInstruction("MULTIPLY", offset);
    case OpCode::DIVIDE:
        return Debug_SimpleInstruction("DIVIDE", offset);
    default:
        printf("Unknown opcode %d\n", instruction);
        return offset + 1;
    }
}

void Debug_DisassembleChunk(Chunk *chunk, const char *name)
{
    printf("== %s ==\n", name);

    for (int offset = 0; offset < chunk->bytecode->size();)
    {
        offset = Debug_DisassembleInstruction(chunk, offset);
    }
}


/// VM

#define STACK_MAX 4000

struct VM
{
    Chunk *chunk;
    u8 *ip; // instruction pointer
    TValue stack[STACK_MAX];
    TValue *sp; // stack pointer
};

VM vm;

void Stack_Reset()
{
    vm.sp = vm.stack;
}

void Stack_Push(TValue value)
{
    *vm.sp = value;
    ++vm.sp;
}

TValue Stack_Pop()
{
    --vm.sp;
    return *vm.sp;
}

void InitVM()
{
    Stack_Reset();
}

void FreeVM()
{

}

#define DEBUG_TRACE_EXECUTION

static InterpretResult Run()
{
#define VM_READ_BYTE() (*vm.ip++) // read byte and move pointer along
#define VM_READ_CONSTANT() (vm.chunk->constants->at(VM_READ_BYTE()))
#define VM_READ_CONSTANT_LONG() (vm.chunk->constants->at(VM_READ_BYTE() << 16 | VM_READ_BYTE() << 8 | VM_READ_BYTE()))
#define VM_BINARY_OP(op) \
    do { \
      double r = Stack_Pop().real; \
      double l = Stack_Pop().real; \
      TValue v; \
      v.real = l op r; \
      Stack_Push(v); \
    } while (false)


    for (;;)
    {
#ifdef DEBUG_TRACE_EXECUTION
        printf("          ");
        for (TValue *slot = vm.stack; slot < vm.sp; ++slot)
        {
            printf("[ ");
            PrintTValue(*slot);
            printf(" ]");
        }
        printf("\n");
        Debug_DisassembleInstruction(vm.chunk, (int)(vm.ip - vm.chunk->bytecode->data()));
#endif
        OpCode op;
        switch (op = (OpCode)VM_READ_BYTE())
        {

            case OpCode::RETURN:
            {
                PrintTValue(Stack_Pop());
                printf("\n");
                return InterpretResult::OK;
            }

            case OpCode::CONSTANT:
            {
                TValue constant = VM_READ_CONSTANT();
                Stack_Push(constant);
                break;
            }

            case OpCode::CONSTANT_LONG:
            {
                TValue constant = VM_READ_CONSTANT_LONG();
                Stack_Push(constant);
                break;
            }

            case OpCode::NEGATE:
            {
                TValue v = Stack_Pop();
                v.real = -v.real;
                Stack_Push(v);
                break;
            }

            case OpCode::ADD: VM_BINARY_OP(+); break;
            case OpCode::SUBTRACT: VM_BINARY_OP(-); break;
            case OpCode::MULTIPLY: VM_BINARY_OP(*); break;
            case OpCode::DIVIDE: VM_BINARY_OP(/); break;

        }
    }


#undef VM_READ_BYTE
#undef VM_READ_CONSTANT
#undef VM_READ_CONSTANT_LONG
#undef VM_BINARY_OP
}

enum TokenType
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
    RETURN,

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
    if (word == "fn") return TokenType::FN;
    if (word == "return") return TokenType::RETURN;

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
        //case '.': return MakeToken(TokenType::DOT);
        case '-': return MakeToken(TokenType::MINUS);
        case '+': return MakeToken(TokenType::PLUS);
        case '/': return MakeToken(TokenType::FORWARDSLASH);
        case '*': return MakeToken(TokenType::ASTERISK);
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

void Compile(const char *source)
{
    InitScanner(source);

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

        if(token.type == TokenType::END_OF_FILE) break;
    }

    //vm.chunk = chunk;
    //vm.ip = vm.chunk->bytecode->data();
    //return Run();
}

InterpretResult Interpret(const char *source)
{
    Compile(source);
    return InterpretResult::OK;
}




void PipLangRunSomeThings()
{
    InitVM();

    Chunk chunk;
    InitChunk(&chunk);

    //TValue v;
    //v.real = 34.201;
    //WriteConstant(&chunk, v, 0);
    //WriteConstant(&chunk, v, 1);
    //WriteChunk(&chunk, (u8)OpCode::NEGATE, 1);
    //WriteChunk(&chunk, (u8)OpCode::RETURN, 2);

    TValue a, b, c;
    a.real = 1.2;
    b.real = 3.4;
    c.real = 5.6;

    WriteConstant(&chunk, a, 0);
    WriteConstant(&chunk, b, 0);
    WriteChunk(&chunk, (u8)OpCode::ADD, 0);
    WriteConstant(&chunk, c, 0);
    WriteChunk(&chunk, (u8)OpCode::DIVIDE, 0);
    WriteChunk(&chunk, (u8)OpCode::NEGATE, 0);
    WriteChunk(&chunk, (u8)OpCode::RETURN, 1);

    //Debug_DisassembleChunk(&chunk, "test chunk");
    //Interpret(&chunk);

    FreeVM();
    FreeChunk(&chunk);
}











