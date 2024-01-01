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

struct TokenSequence
{
    Token *tokens = NULL;
    //Token tokens[4096];
    int numTokens = 0;
    int cursor = 0;

    void Allocate()
    {
        numTokens = 0;
        cursor = 0;
        if (tokens == NULL)
            tokens = (Token*)calloc(4096, sizeof(Token));
    }

    void Free()
    {
        free(tokens);
        numTokens = 0;
        cursor = 0;
    }
};

TokenSequence tokensequence;

struct Parser
{
    Token current{};
    Token previous{};
    Token previousprevious{};

    bool hadError = false;
    bool panicMode = false;
    bool previewMode = false;

    int cachedCursorPosBeforePreview = 0;
    void TurnOnPreviewMode()
    {
        if (previewMode) PipLangAssert(0);
        cachedCursorPosBeforePreview = tokensequence.cursor;
        previewMode = true;
    }

    void TurnOffPreviewMode()
    {
        if (!previewMode) PipLangAssert(0);
        tokensequence.cursor = cachedCursorPosBeforePreview;
        current = tokensequence.cursor < 1 ? Token() : tokensequence.tokens[tokensequence.cursor - 1];
        previous = tokensequence.cursor < 2 ? Token() : tokensequence.tokens[tokensequence.cursor - 2];
        previousprevious = tokensequence.cursor < 3 ? Token() : tokensequence.tokens[tokensequence.cursor - 3];
        previewMode = false;
    }

    bool SweepPreviewFor(TokenType type) const
    {
        PipLangAssert(cachedCursorPosBeforePreview > 0);
        for (int i = cachedCursorPosBeforePreview - 1; i < tokensequence.cursor; ++i)
            if (tokensequence.tokens[i].type == type) return true;
        return false;
    }
};

struct Local
{
    Token name;
    int depth;
};

enum class CompilingToType
{
    FUNCTION,
    TOPLEVELSCRIPT
};

struct Compiler
{
    Compiler *enclosing;
    PipFunction *compilingTo;
    CompilingToType compilingToType;

    Local locals[256];
    int localCount;
    int scopeDepth;
};

Parser parser;
Compiler *current = NULL;

static void InitCompiler(Compiler *compiler, CompilingToType compilingToType)
{
    if (parser.previewMode) PipLangAssert(0);

    compiler->enclosing = current;
    compiler->compilingTo = NULL;
    compiler->compilingToType = compilingToType;

    compiler->localCount = 0;
    compiler->scopeDepth = 0;
    compiler->compilingTo = NewFunction();
    current = compiler;
    if (compilingToType != CompilingToType::TOPLEVELSCRIPT)
    {
        current->compilingTo->name = CopyString(parser.previous.start, parser.previous.length, false);
    }

    // Compiler claims stack slot zero for class methods but I'm probably not going to implement classes or methods
    Local *local = &current->locals[current->localCount++];
    local->depth = 0;
    local->name.start = "";
    local->name.length = 0;
}

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
    parser.previousprevious = parser.previous;
    parser.previous = parser.current;
    parser.current = tokensequence.tokens[tokensequence.cursor++];
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

static bool Check(TokenType type)
{
    return parser.current.type == type;
}

static bool Match(TokenType type)
{
    if (!Check(type)) return false;
    Advance();
    return true;
}

static void EscapePanicMode()
{
    parser.panicMode = false;

    while (parser.current.type != TokenType::END_OF_FILE) 
    {
        switch (parser.current.type) 
        {
            case TokenType::FN:
            case TokenType::MUT:
            case TokenType::FOR:
            case TokenType::WHILE:
            case TokenType::IF:
            case TokenType::RETURN:
                return;
            default:;
        }
        Advance();
    }
}

static Chunk *CurrentChunk()
{
    return &current->compilingTo->chunk;
}

static void EmitByte(u8 byte)
{
    if (parser.previewMode) return;

    WriteChunk(CurrentChunk(), byte, parser.previous.line);
}

static void EmitByte(OpCode op)
{
    if (parser.previewMode) return;

    EmitByte((u8)op);
}

static void EmitConstant(TValue value)
{
    if (parser.previewMode) return;

    WriteConstant(CurrentChunk(), value, parser.previous.line);
}

static void EmitBytes(u8 byte1, u8 byte2)
{
    if (parser.previewMode) return;

    EmitByte(byte1);
    EmitByte(byte2);
}

static void PatchJump(int index)
{
    if (parser.previewMode) return;

    u16 jump = (u16)((int)CurrentChunk()->bytecode->size() - index - 2); // 2 bytecodes for jump offset

    if (jump > UINT16_MAX) 
    {
        Error("Too much code to jump over. Reduce code in block.");
    }

    CurrentChunk()->bytecode->at(index)     = (jump >> 8) & 0xff;
    CurrentChunk()->bytecode->at(index + 1) = jump & 0xff;
}

static int EmitJump(OpCode op)
{
    if (parser.previewMode) return -1;

    EmitByte(op);
    EmitByte(0xff); // return address/index of this byte
    EmitByte(0xff);
    return (int)CurrentChunk()->bytecode->size() - 2;
}

static void EmitLoop(int loopStart)
{
    if (parser.previewMode) return;

    EmitByte(OpCode::JUMP_BACK);

    u16 jump = (u16)((int)CurrentChunk()->bytecode->size() - loopStart + 2);

    if (jump > UINT16_MAX)
    {
        Error("Too much code to jump over. Reduce code in block.");
    }

    EmitByte((jump >> 8) & 0xff);
    EmitByte(jump & 0xff);
}

static PipFunction *EndCompiler()
{
    if (parser.previewMode) PipLangAssert(0);

    EmitByte(OpCode::OP_FALSE);
    EmitByte(OpCode::RETURN);

    PipFunction *fn = current->compilingTo;

#ifdef DEBUG_PRINT_CODE
    if (!parser.hadError)
    {
        DisassembleChunk(CurrentChunk(), fn->name != NULL ? fn->name->text.c_str() : "<pip top-level script>");
    }
#endif

    current = current->enclosing;
    return fn;
}


static void Expression();

static void HashMapLiteral()
{
    EmitByte(OpCode::NEW_HASHMAP);

    while (!Match(TokenType::RBRACE))
    {
        Expression(); // key expression
        Eat(TokenType::COLON, "Expected ':' after Key value expression in Map initializer.");
        Expression(); // value expression
        EmitByte(OpCode::SET_MAP_ENTRY);
        if (!Check(TokenType::RBRACE))
            Eat(TokenType::COMMA, "Expected ',' between map entry initializers.");
    }
}

static void NumberLiteral()
{
    double value = strtod(parser.previous.start, NULL);
    EmitConstant(NUMBER_VAL(value));
}

static void StringLiteral()
{
    EmitConstant(RCOBJ_VAL((RCObject*)CopyString(parser.previous.start + 1, parser.previous.length - 2, true)));
}

static void Expression()
{
    ParsePrecedence(Precedence::OR);
}

static void Grouping()
{
    Expression();
    Eat(TokenType::RPAREN, "Expected ')' after expression.");
}

static void Unary()
{
    TokenType operatorType = parser.previous.type;

    ParsePrecedence(Precedence::UNARY); // Compile the operand

    switch (operatorType)
    {
        case TokenType::BANG:  EmitByte(OpCode::LOGICAL_NOT); break;
        case TokenType::MINUS: EmitByte(OpCode::NEGATE); break;
    }
}

static u8 ArgumentList()
{
    u8 argc = 0;
    if (!Check(TokenType::RPAREN))
    {
        do 
        {
            Expression();
            EmitByte(OpCode::INCREMENT_REF_IF_RCOBJ);
            if (argc == 255) Error("Can't have more than 255 arguments to a function.");
            ++argc;
        } while (Match(TokenType::COMMA));
    }
    Eat(TokenType::RPAREN, "Expected ')' after function arguments.");
    return argc;
}

static void Call()
{
    u8 argc = ArgumentList();
    EmitByte(OpCode::CALL);
    EmitByte(argc);
}

static void BinOp()
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

static void BoolLiteral()
{
    switch (parser.previous.type) 
    {
        case TokenType::TRUE: EmitByte((u8)OpCode::OP_TRUE); break;
        case TokenType::FALSE: EmitByte((u8)OpCode::OP_FALSE); break;
    }
}

static void LogicalAnd()
{
    int endJump = EmitJump(OpCode::JUMP_IF_FALSE);

    EmitByte(OpCode::POP);
    ParsePrecedence(Precedence::AND);
    
    PatchJump(endJump);
}

static void LogicalOr()
{
    int elseJump = EmitJump(OpCode::JUMP_IF_FALSE);
    int endJump = EmitJump(OpCode::JUMP);
    PatchJump(elseJump); // go here to check rest of predicate if False
    EmitByte(OpCode::POP);
    ParsePrecedence(Precedence::OR);
    PatchJump(endJump); // go here to skip rest of predicate if True
}

static u32 IdentifierConstant(Token *name)
{
    if (parser.previewMode) PipLangAssert(0);

    return AddConstant(CurrentChunk(), RCOBJ_VAL((RCObject *)CopyString(name->start, name->length, true)));
}

static bool IdentifiersEqual(Token *a, Token *b)
{
    if (a->length != b->length) return false;
    return memcmp(a->start, b->start, a->length) == 0;
}

static int ResolveLocal(Compiler *compiler, Token *name)
{
    if (parser.previewMode) PipLangAssert(0);

    // Requires: statements have stack effect of zero (it does not increase or decrease stack size).
    // Except for when local variables are initialized. This ensures that the n-th local variable will
    // be located exactly at stack[n].
    for (int i = compiler->localCount - 1; i >= 0; --i)
    {
        Local *local = &compiler->locals[i];
        if (IdentifiersEqual(name, &local->name))
        {
            if (local->depth == -1)
            {
                Error("Can't read local variable in its own initializer");
            }
            return i;
        }
    }
    return -1;
}

static void NamedVariable(Token name)
{
    if (parser.previewMode) PipLangAssert(0);

    if (Check(TokenType::EQUAL))
    {
        return; // then Assignment Effect
    }

    int localIndexResolved = ResolveLocal(current, &name);

    if (localIndexResolved == -1)
    {
        u32 arg = IdentifierConstant(&name);
        EmitByte(OpCode::GET_GLOBAL);
        EmitByte((u8)(arg >> 16));
        EmitByte((u8)(arg >> 8));
        EmitByte((u8)(arg));
    }
    else
    {
        u8 arg = (u8)localIndexResolved;
        EmitByte(OpCode::GET_LOCAL);
        EmitByte(arg);
    }
}

static void Variable()
{
    if (parser.previewMode) return;
    NamedVariable(parser.previous);
}

static void Dot()
{
    Eat(TokenType::IDENTIFIER, "Expected map entry name after '.'.");

    if (Check(TokenType::EQUAL))
    {
        return; // then Assignment Effect
    }

    Token id = parser.previous;
    auto str = std::string(id.start, id.length);
    if (str == "insert" || str == "append" || str == "remove")
    {
        if (str == "insert")
        {
            Eat(TokenType::LPAREN, "Expected '(' after 'insert' contextual keyword.");
            Expression(); // Key
            Eat(TokenType::COMMA, "Expected ',' after Key expression of 'insert' contextual keyword.");
            Expression();
            Eat(TokenType::RPAREN, "Expected ')' after Value expression of 'insert' contextual keyword.");
            EmitByte(OpCode::SET_MAP_ENTRY);
        }
        else if (str == "append")
        {

        }
        else if (str == "remove")
        {
            Eat(TokenType::LPAREN, "Expected '(' after 'remove' contextual keyword.");
            Expression(); // Key
            Eat(TokenType::RPAREN, "Expected ')' after Key expression of 'remove' contextual keyword.");
            EmitByte(OpCode::DEL_MAP_ENTRY);
        }
    }
    else
    {
        // Otherwise GET_MAP_ENTRY
        EmitConstant(RCOBJ_VAL((RCObject *)CopyString(id.start, id.length, true)));
        EmitByte(OpCode::GET_MAP_ENTRY);
    }
}


static void Effect();
static void Statement();
static void Declaration();
static void ParseVariableDeclaration();

static void Block()
{
    while (!Check(TokenType::RBRACE) && !Check(TokenType::END_OF_FILE))
    {
        Declaration();
    }

    Eat(TokenType::RBRACE, "Expected '}' after block.");
}

static void BeginScope()
{
    current->scopeDepth++;
}

static void EndScope()
{
    current->scopeDepth--;

    while (current->localCount > 0 &&
           current->locals[current->localCount - 1].depth > current->scopeDepth)
    {
        /* TODO 
        When multiple local variables go out of scope at once, you get a series of OP_POP instructions that get interpreted one at a time. 
        A simple optimization you could add is a specialized OP_POPN instruction that takes an operand for the number of slots to pop and 
        pops them all at once.
        */
        EmitByte(OpCode::POP_LOCAL);
        current->localCount--;
    }
}

static void IfStatement()
{
    if (parser.previewMode) PipLangAssert(0);

    Eat(TokenType::LPAREN, "Expected '(' after 'if'.");
    Expression();
    Eat(TokenType::RPAREN, "Expected ')' after predicate.");
    
    int thenJump = EmitJump(OpCode::JUMP_IF_FALSE);
    
    EmitByte(OpCode::POP);
    Statement(); // if-body Block
    int elseJump = EmitJump(OpCode::JUMP);

    PatchJump(thenJump);

    EmitByte(OpCode::POP);

    if (Match(TokenType::ELSE)) Statement(); // else-body Block

    PatchJump(elseJump);
}

static void WhileStatement()
{
    if (parser.previewMode) PipLangAssert(0);

    Eat(TokenType::LPAREN, "Expected '(' after keyword 'while'.");
    int loopStart = (int)CurrentChunk()->bytecode->size();
    Expression();
    Eat(TokenType::RPAREN, "Expected ')' after while-loop predicate.");

    int exitJump = EmitJump(OpCode::JUMP_IF_FALSE);
    EmitByte(OpCode::POP);
    Statement(); // while-body Block
    EmitLoop(loopStart);
    PatchJump(exitJump);
    EmitByte(OpCode::POP);
}

static void ForStatement()
{
    if (parser.previewMode) PipLangAssert(0);

    BeginScope();

    Eat(TokenType::LPAREN, "Expected '(' after 'for'.");
    if (Match(TokenType::COMMA))
    {
        // no initializer
    }
    else if (Match(TokenType::MUT))
    {
        ParseVariableDeclaration();
    }
    else
    {
        Effect();
    }
    Eat(TokenType::COMMA, "Expected ','.");
    int loopStart = (int)CurrentChunk()->bytecode->size();
    int exitJump = -1;
    if (!Match(TokenType::COMMA))
    {
        Expression();
        Eat(TokenType::COMMA, "Expected ',' after for-loop predicate.");
        exitJump = EmitJump(OpCode::JUMP_IF_FALSE);
        EmitByte(OpCode::POP);
    }
    if (!Match(TokenType::RPAREN))
    {
        int bodyJump = EmitJump(OpCode::JUMP);
        int incrementStart = (int)CurrentChunk()->bytecode->size();
        Effect();
        Eat(TokenType::RPAREN, "Expected ')' after for-loop clauses.");

        EmitLoop(loopStart);
        loopStart = incrementStart;
        PatchJump(bodyJump);
    }

    Statement(); // for-loop Block

    EmitLoop(loopStart);

    if (exitJump != -1)
    {
        PatchJump(exitJump);
        EmitByte(OpCode::POP);
    }

    EndScope();
}

static void ReturnStatement()
{
    if (parser.previewMode) PipLangAssert(0);

    if (current->compilingToType == CompilingToType::TOPLEVELSCRIPT)
    {
        Error("Can't return from top-level script.");
    }

    Eat(TokenType::LPAREN, "Expected '(' after 'return' keyword.");
    if (Match(TokenType::RPAREN))
    {
        EmitByte(OpCode::OP_FALSE);
        EmitByte(OpCode::RETURN);
    }
    else
    {
        Expression();
        Eat(TokenType::RPAREN, "Expected ')' after return value.");
        EmitByte(OpCode::RETURN);
    }
}

static void PrintStatement()
{
    if (parser.previewMode) PipLangAssert(0);

    Eat(TokenType::LPAREN, "Expected '(' after 'print' keyword.");
    if (!Match(TokenType::RPAREN))
    {
        Expression();
        Eat(TokenType::RPAREN, "Expected ')' after print value.");
        EmitByte(OpCode::PRINT);
    }
}

static void Effect() // Stack effect = 0
{
    if (parser.previewMode) PipLangAssert(0);

    parser.TurnOnPreviewMode();
    Expression();
    if (Check(TokenType::EQUAL)) // ASSIGNMENT EFFECT
    {
        parser.TurnOffPreviewMode();

        Expression();

        if (parser.previous.type != TokenType::IDENTIFIER)
        {
            ErrorAtCurrent("Left of assignment operator must be an identifier.");
        }

        if (parser.previousprevious.type == TokenType::DOT) // set map entry
        {
            Token id = parser.previous;
            EmitConstant(RCOBJ_VAL((RCObject *)CopyString(id.start, id.length, true)));
            PipLangAssert(Match(TokenType::EQUAL));
            Expression();
            EmitByte(OpCode::SET_MAP_ENTRY);
            EmitByte(OpCode::POP);
        }
        else // set local or global var
        {
            int localIndexResolved = ResolveLocal(current, &parser.previous);

            if (localIndexResolved == -1)
            {
                u32 arg = IdentifierConstant(&parser.previous);

                PipLangAssert(Match(TokenType::EQUAL));
                Expression();

                EmitByte(OpCode::SET_GLOBAL);
                EmitByte((u8)(arg >> 16));
                EmitByte((u8)(arg >> 8));
                EmitByte((u8)(arg));
            }
            else
            {
                u8 arg = (u8)localIndexResolved;

                PipLangAssert(Match(TokenType::EQUAL));
                Expression();

                EmitByte(OpCode::SET_LOCAL);
                EmitByte(arg);
            }
        }
    }
    else // EXPRESSION EFFECT
    {
        parser.TurnOffPreviewMode();
        Expression();
        EmitByte(OpCode::POP);
    }
}

static void Statement()
{
    if (parser.previewMode) PipLangAssert(0);

    if (Match(TokenType::LBRACE))
    {
        BeginScope();
        Block();
        EndScope();
    }
    else if (Match(TokenType::IF))
    {
        IfStatement();
    }
    else if (Match(TokenType::WHILE))
    {
        WhileStatement();
    }
    else if (Match(TokenType::FOR))
    {
        ForStatement();
    }
    else if (Match(TokenType::RETURN))
    {
        ReturnStatement();
    }
    else if (Match(TokenType::PRINT))
    {
        PrintStatement();
    }
    else
    {
        Effect();
    }
}

// Add local variable to the compiler's list of variables for the current scope
static void AddLocal(Token name)
{
    if (parser.previewMode) PipLangAssert(0);

    if (current->localCount == 256)
    {
        Error("Functions can only declare 256 local variables. Too many local variables in function.");
        return;
    }

    Local *local = &current->locals[current->localCount++];
    local->name = name;
    local->depth = -1;
}

// Record the existence of a local variable
static void DeclareLocalVariable()
{
    if (parser.previewMode) PipLangAssert(0);
    if (current->scopeDepth == 0) return;

    Token *name = &parser.previous;

    for (int i = current->localCount - 1; i >= 0; --i)
    {
        Local *local = &current->locals[i];
        if (local->depth != -1 && local->depth < current->scopeDepth)
            break;
        if (IdentifiersEqual(name, &local->name))
            Error("Already a variable with this name in this scope.");
    }

    AddLocal(*name);
}

static u32 ParseVariable(const char * errormsg)
{
    if (parser.previewMode) PipLangAssert(0);

    Eat(TokenType::IDENTIFIER, errormsg);

    DeclareLocalVariable();
    if (current->scopeDepth > 0) return 0;

    return IdentifierConstant(&parser.previous);
}

static void MarkLocalAsInitialized()
{
    current->locals[current->localCount - 1].depth = current->scopeDepth;
}

static void DefineVariable(u32 global)
{
    if (parser.previewMode) PipLangAssert(0);

    if (current->scopeDepth > 0)
    {
        MarkLocalAsInitialized();
        return;
    }

    // TODO(Kevin): Optimize by using LONG op if first 24 bits are non zero or just use u8 and limit max global varibles to 256
    EmitByte(OpCode::DEFINE_GLOBAL);
    EmitByte((u8)(global >> 16));
    EmitByte((u8)(global >> 8));
    EmitByte((u8)(global));
}

static void ParseVariableDeclaration()
{
    if (parser.previewMode) PipLangAssert(0);

    u32 global = ParseVariable("Expected variable identifier after 'mut'.");

    if (Match(TokenType::EQUAL))
    {
        Expression();
        if (current->scopeDepth > 0)
        {
            // check if result of expression is RCOBJ, then increment its ref
            EmitByte(OpCode::INCREMENT_REF_IF_RCOBJ);
        }
    }
    else
    {
        // variable declaration without initializing? probably want this
    }

    DefineVariable(global);
}

static void Function(CompilingToType compilingToType)
{
    if (parser.previewMode) PipLangAssert(0);

    Compiler compiler;
    InitCompiler(&compiler, compilingToType);

    BeginScope();
    Eat(TokenType::LPAREN, "Expect '(' after function name.");
    // Semantically, a parameter is simply a local variable declared in the outermost lexical scope of the function body. 
    // We get to use the existing compiler support for declaring named local variables to parse and compile parameters. 
    // Unlike local variables, which have initializers, there’s no code here to initialize the parameter’s value.
    if (!Check(TokenType::RPAREN)) 
    {
        do {
            current->compilingTo->arity++;
            if (current->compilingTo->arity > 255) 
            {
                ErrorAtCurrent("Can't have more than 255 parameters.");
            }
            ParseVariable("Expect parameter name.");
            DefineVariable(0);
        } while (Match(TokenType::COMMA));
    }
    Eat(TokenType::RPAREN, "Expect ')' after parameters.");
    Eat(TokenType::LBRACE, "Expect '{' before function body.");
    Block();

    PipFunction *fn = EndCompiler();

    u32 arg = AddConstant(CurrentChunk(), FUNCTION_VAL(fn));
    EmitByte(OpCode::CONSTANT_LONG);
    EmitByte((u8)(arg >> 16));
    EmitByte((u8)(arg >> 8));
    EmitByte((u8)(arg));
}

static void ParseFunctionDeclaration()
{
    if (parser.previewMode) PipLangAssert(0);
    if (current->scopeDepth > 0) Error("Functions must be declared at the top-level.");

    u32 global = ParseVariable("Expected function identifier after 'fn'.");
    Function(CompilingToType::FUNCTION);
    DefineVariable(global);
}

static void Declaration()
{
    if (parser.previewMode) PipLangAssert(0);

    if (Match(TokenType::MUT))
    {
        ParseVariableDeclaration();
    }
    else if (Match(TokenType::FN))
    {
        ParseFunctionDeclaration();
    }
    else
    {
        Statement();
    }
    
    if (parser.panicMode) EscapePanicMode();
}


ParseRule rules[(u8)TokenType::END_OF_FILE];

void SetupParsingRules()
{
    rules[(u8)TokenType::LESS]              = {          NULL,        BinOp, Precedence::COMPARISON };
    rules[(u8)TokenType::LESS_EQUAL]        = {          NULL,        BinOp, Precedence::COMPARISON };
    rules[(u8)TokenType::GREATER]           = {          NULL,        BinOp, Precedence::COMPARISON };
    rules[(u8)TokenType::GREATER_EQUAL]     = {          NULL,        BinOp, Precedence::COMPARISON };
    rules[(u8)TokenType::EQUAL]             = {          NULL,         NULL, Precedence::NONE };
    rules[(u8)TokenType::BANG_EQUAL]        = {          NULL,        BinOp, Precedence::EQUALITY };
    rules[(u8)TokenType::EQUAL_EQUAL]       = {          NULL,        BinOp, Precedence::EQUALITY };
    rules[(u8)TokenType::BANG]              = {         Unary,         NULL, Precedence::NONE };
    rules[(u8)TokenType::LSQBRACK]          = {  /*ArrayLiteral*/NULL,  /*AccessAorM*/NULL, Precedence::NONE /*CALL probably*/ };
    rules[(u8)TokenType::RSQBRACK]          = {          NULL,         NULL, Precedence::NONE };
    rules[(u8)TokenType::LPAREN]            = {      Grouping,         Call, Precedence::CALL };
    rules[(u8)TokenType::RPAREN]            = {          NULL,         NULL, Precedence::NONE };
    rules[(u8)TokenType::LBRACE]            = {HashMapLiteral,         NULL, Precedence::NONE };
    rules[(u8)TokenType::RBRACE]            = {          NULL,         NULL, Precedence::NONE };
    rules[(u8)TokenType::COMMA]             = {          NULL,         NULL, Precedence::NONE };
    rules[(u8)TokenType::DOT]               = {          NULL,          Dot, Precedence::CALL };
    rules[(u8)TokenType::PLUS]              = {          NULL,        BinOp, Precedence::TERM };
    rules[(u8)TokenType::MINUS]             = {         Unary,        BinOp, Precedence::TERM };
    rules[(u8)TokenType::ASTERISK]          = {          NULL,        BinOp, Precedence::FACTOR };
    rules[(u8)TokenType::FORWARDSLASH]      = {          NULL,        BinOp, Precedence::FACTOR };

    rules[(u8)TokenType::NUMBER_LITERAL]    = { NumberLiteral,         NULL, Precedence::NONE };
    rules[(u8)TokenType::STRING_LITERAL]    = { StringLiteral,         NULL, Precedence::NONE };
    rules[(u8)TokenType::IDENTIFIER]        = {      Variable,         NULL, Precedence::NONE };
    
    rules[(u8)TokenType::TRUE]              = {   BoolLiteral,         NULL, Precedence::NONE };
    rules[(u8)TokenType::FALSE]             = {   BoolLiteral,         NULL, Precedence::NONE };
    rules[(u8)TokenType::AND]               = {          NULL,   LogicalAnd, Precedence::AND };
    rules[(u8)TokenType::OR]                = {          NULL,    LogicalOr, Precedence::OR };
    rules[(u8)TokenType::FN]                = {          NULL,         NULL, Precedence::NONE };
    rules[(u8)TokenType::IF]                = {          NULL,         NULL, Precedence::NONE };
    rules[(u8)TokenType::ELSE]              = {          NULL,         NULL, Precedence::NONE };
    rules[(u8)TokenType::WHILE]             = {          NULL,         NULL, Precedence::NONE };
    rules[(u8)TokenType::FOR]               = {          NULL,         NULL, Precedence::NONE };
    rules[(u8)TokenType::MUT]               = {          NULL,         NULL, Precedence::NONE };
    rules[(u8)TokenType::RETURN]            = {          NULL,         NULL, Precedence::NONE };
    rules[(u8)TokenType::PRINT]             = {          NULL,         NULL, Precedence::NONE };
    
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

static void TokenizeAll(const char *source)
{
    InitScanner(source);

    for (int i = 0; i < 4096; ++i)
    {
        Token t = ScanToken();

        if (t.type == TokenType::ERROR)
        {
            ErrorAt(&t, t.start);
            break;
        }

        tokensequence.tokens[i] = t;

        if (t.type == TokenType::END_OF_FILE)
        {
            tokensequence.numTokens = i + 1;
            break;
        }
    }
}

PipFunction *Compile(const char *source)
{
    tokensequence.Allocate();

    SetupParsingRules();

    parser.hadError = false;
    parser.panicMode = false;
    TokenizeAll(source);
    if (parser.hadError) return NULL;

    Compiler compiler;
    InitCompiler(&compiler, CompilingToType::TOPLEVELSCRIPT);

    Advance();

    while (!Match(TokenType::END_OF_FILE))
    {
        Declaration();
    }

    PipFunction *toplevelscriptfn = EndCompiler();
    return parser.hadError ? NULL : toplevelscriptfn;

    tokensequence.Free();
}
