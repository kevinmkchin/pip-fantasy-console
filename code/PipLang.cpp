#include "PipLang.h"

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

enum class InterpretResult 
{
    OK,
    COMPILE_ERROR,
    RUNTIME_ERROR
};

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

InterpretResult Interpret(Chunk *chunk)
{
    vm.chunk = chunk;
    vm.ip = vm.chunk->bytecode->data();
    return Run();
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
    Interpret(&chunk);

    FreeVM();
    FreeChunk(&chunk);
}











