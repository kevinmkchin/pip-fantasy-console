#include "VM.h"

#include <stdio.h>

#include "Debug.h"
#include "Chunk.h"
#include "Scanner.h"
#include "Compiler.h"


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
        DisassembleInstruction(vm.chunk, (int)(vm.ip - vm.chunk->bytecode->data()));
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











