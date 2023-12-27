#include "VM.h"

#include <stdio.h>
#include <stdarg.h>

#include "Debug.h"
#include "Chunk.h"
#include "Scanner.h"
#include "Compiler.h"
#include "Object.h"

/// VM

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

TValue Stack_Peek(int distance)
{
    return vm.sp[-1 - distance];
}

void InitVM()
{
    Stack_Reset();
    InitHashMap(&vm.interned_strings);
    InitHashMap(&vm.globals);
}

void FreeVM()
{
    FreeHashMap(&vm.interned_strings);
    FreeHashMap(&vm.globals);
}

static void RuntimeError(const char *format, ...)
{
    size_t instruction = vm.ip - vm.chunk->bytecode->data() - 1;
    int line = vm.chunk->linenumbers->at(instruction);
    fprintf(stderr, "[line %d] Runtime error: ", line);

    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    Stack_Reset();
}

static bool IsFalsey(TValue v)
{
    return !AS_BOOL(v);
}

static void ConcatenateStrings()
{
    RCString *r = RCOBJ_AS_STRING(Stack_Pop());
    RCString *l = RCOBJ_AS_STRING(Stack_Pop());
    std::string temp = (l->text + r->text);
    Stack_Push(RCOBJ_VAL((RCObject*)CopyString(temp.c_str(), (int)temp.size())));
}

static bool IsEqual(TValue l, TValue r)
{
    if (l.type != r.type) 
        return false;

    switch (l.type) 
    {
        case TValue::BOOLEAN: return AS_BOOL(l) == AS_BOOL(r);
        case TValue::REAL:    return AS_NUMBER(l) == AS_NUMBER(r);
        case TValue::RCOBJ:
        {
            // TODO the other RCOBJ types
            return RCOBJ_AS_STRING(l) == RCOBJ_AS_STRING(r);
        }
        default: return false;
    }
}

static InterpretResult Run()
{
#define VM_READ_BYTE() (*vm.ip++) // read byte and move pointer along
#define VM_READ_CONSTANT() (vm.chunk->constants->at(VM_READ_BYTE()))
#define VM_READ_CONSTANT_LONG() (vm.chunk->constants->at(  (u32)*vm.ip++ << 16 | (u32)*(vm.ip++ + 1) << 8 | (u32)*(vm.ip++ + 2)  )) // 2023-12-27 have to do it this was because postfix increments happen at the very very end...
#define VM_BINARY_OP(resultValueConstructor, op) \
    do { \
        if (!IS_NUMBER(Stack_Peek(0)) || !IS_NUMBER(Stack_Peek(1))) \
        { \
            RuntimeError("Operands to BINOP must be number values."); \
            return InterpretResult::RUNTIME_ERROR; \
        } \
        double r = Stack_Pop().real; \
        double l = Stack_Pop().real; \
        Stack_Push(resultValueConstructor(l op r)); \
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
        DisassembleInstruction(vm.chunk, (int)(vm.ip - vm.chunk->bytecode->data()));
#endif
        OpCode op;
        switch (op = (OpCode)VM_READ_BYTE())
        {

            case OpCode::RETURN:
                return InterpretResult::OK;

            case OpCode::CONSTANT:
                Stack_Push(VM_READ_CONSTANT());
                break;

            case OpCode::CONSTANT_LONG:
                Stack_Push(VM_READ_CONSTANT_LONG());
                break;

            case OpCode::POP: Stack_Pop(); break; // TODO(Kevin): destroy transient?

            case OpCode::DEFINE_GLOBAL:
            {
                RCString *name = RCOBJ_AS_STRING(VM_READ_CONSTANT_LONG());
                HashMapSet(&vm.globals, name, Stack_Peek(0));
                Stack_Pop();
                break;
            }

            case OpCode::GET_GLOBAL:
            {
                RCString *name = RCOBJ_AS_STRING(VM_READ_CONSTANT_LONG());
                TValue value;
                if (!HashMapGet(&vm.globals, name, &value))
                {
                    RuntimeError("Undefined variable '%s'.", name->text.c_str());
                    return InterpretResult::RUNTIME_ERROR;
                }
                Stack_Push(value);
                break;
            }

            case OpCode::SET_GLOBAL:
            {
                RCString *name = RCOBJ_AS_STRING(VM_READ_CONSTANT_LONG());
                if (HashMapSet(&vm.globals, name, Stack_Peek(0)))
                {
                    HashMapDelete(&vm.globals, name);
                    RuntimeError("Undefined variable '%s'.", name->text.c_str());
                    return InterpretResult::RUNTIME_ERROR;
                }
                Stack_Pop();
                break;
            }

            case OpCode::NEGATE:
            {
                if (!IS_NUMBER(Stack_Peek(0)))
                {
                    RuntimeError("Operand to NEGATE op must be a number value.");
                    return InterpretResult::RUNTIME_ERROR;
                }
                Stack_Push(TValue::Number(-AS_NUMBER(Stack_Pop())));
                break;
            }
            case OpCode::ADD: 
            {
                if (RCOBJ_IS_STRING(Stack_Peek(0)) && RCOBJ_IS_STRING(Stack_Peek(1)))
                {
                    ConcatenateStrings();
                }
                else if (IS_NUMBER(Stack_Peek(0)) && IS_NUMBER(Stack_Peek(1)))
                {
                    double r = Stack_Pop().real;
                    double l = Stack_Pop().real;
                    Stack_Push(NUMBER_VAL(l + r));
                }
                else
                {
                    RuntimeError("Operands to BINOP must be number values.");
                    return InterpretResult::RUNTIME_ERROR;
                }
                break;
            }
            case OpCode::SUBTRACT: VM_BINARY_OP(NUMBER_VAL, -); break;
            case OpCode::MULTIPLY: VM_BINARY_OP(NUMBER_VAL, *); break;
            case OpCode::DIVIDE: VM_BINARY_OP(NUMBER_VAL, /); break;

            case OpCode::OP_TRUE: Stack_Push(BOOL_VAL(true)); break;
            case OpCode::OP_FALSE: Stack_Push(BOOL_VAL(false)); break;
            case OpCode::LOGICAL_NOT:
                if (!IS_BOOL(Stack_Peek(0)))
                {
                    RuntimeError("Operand to LOGICAL NOT op must be a boolean value.");
                    return InterpretResult::RUNTIME_ERROR;
                }
                Stack_Push(BOOL_VAL(IsFalsey(Stack_Pop())));
                break;
            case OpCode::RELOP_EQUAL:
                Stack_Push(BOOL_VAL(IsEqual(Stack_Pop(), Stack_Pop())));
                break;
            case OpCode::RELOP_GREATER: VM_BINARY_OP(BOOL_VAL, >); break;
            case OpCode::RELOP_LESSER: VM_BINARY_OP(BOOL_VAL, <); break;
        }
    }


#undef VM_READ_BYTE
#undef VM_READ_CONSTANT
#undef VM_READ_CONSTANT_LONG
#undef VM_BINARY_OP
}


InterpretResult Interpret(const char *source)
{
    Chunk chunk;
    InitChunk(&chunk);

    if (!Compile(source, &chunk))
    {
        FreeChunk(&chunk);
        return InterpretResult::COMPILE_ERROR;
    }

    vm.chunk = &chunk;
    vm.ip = vm.chunk->bytecode->data();

    InterpretResult result = Run();

    FreeChunk(&chunk);
    return result;
}

InterpretResult PipLangVM_RunScript(const char *source)
{
    InitVM();

    return Interpret(source);

    FreeVM();
}

//void PipLangRunSomeThings()
//{
//    InitVM();
//
//    Chunk chunk;
//    InitChunk(&chunk);
//
//    //TValue v;
//    //v.real = 34.201;
//    //WriteConstant(&chunk, v, 0);
//    //WriteConstant(&chunk, v, 1);
//    //WriteChunk(&chunk, (u8)OpCode::NEGATE, 1);
//    //WriteChunk(&chunk, (u8)OpCode::RETURN, 2);
//
//    TValue a, b, c;
//    a.real = 1.2;
//    b.real = 3.4;
//    c.real = 5.6;
//
//    WriteConstant(&chunk, a, 0);
//    WriteConstant(&chunk, b, 0);
//    WriteChunk(&chunk, (u8)OpCode::ADD, 0);
//    WriteConstant(&chunk, c, 0);
//    WriteChunk(&chunk, (u8)OpCode::DIVIDE, 0);
//    WriteChunk(&chunk, (u8)OpCode::NEGATE, 0);
//    WriteChunk(&chunk, (u8)OpCode::RETURN, 1);
//
//    //Debug_DisassembleChunk(&chunk, "test chunk");
//    //Interpret(&chunk);
//
//    FreeVM();
//    FreeChunk(&chunk);
//}











