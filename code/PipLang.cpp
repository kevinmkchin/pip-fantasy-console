#include "PipLang.h"

#include <vector>
#include <stdio.h>


enum class OpCode : u8
{
    RETURN,
    CONSTANT,
    CONSTANT_LONG
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

void WriteChunk(Chunk *chunk, u8 byte, int line)
{
    chunk->linenumbers->push_back(line);
    chunk->bytecode->push_back(byte);
}

void FreeChunk(Chunk *chunk)
{
    chunk->linenumbers->clear();
    chunk->bytecode->clear();
    chunk->constants->clear();
    // Note(Kevin): I'm choosing not to delete the std::vectors. Keep them alive but just empty.
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
    case OpCode::CONSTANT_LONG:
        return Debug_ConstantLongInstruction("CONSTANT_LONG", chunk, offset);
    case OpCode::CONSTANT:
        return Debug_ConstantInstruction("CONSTANT", chunk, offset);
    case OpCode::RETURN:
        return Debug_SimpleInstruction("RETURN", offset);
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


void WriteConstant(Chunk *chunk, TValue value, int line)
{
    chunk->constants->push_back(value);
    u32 cindex = chunk->constants->size() - 1;
    if (cindex >= 256)
    {
        WriteChunk(chunk, (u8)OpCode::CONSTANT_LONG, line);
        WriteChunk(chunk, (u8)cindex >> 16, line);
        WriteChunk(chunk, (u8)cindex >> 8, line);
        WriteChunk(chunk, (u8)cindex, line);
    }
    else
    {
        WriteChunk(chunk, (u8)OpCode::CONSTANT, line);
        WriteChunk(chunk, (u8)cindex, line);
    }
}

void PipLangRunSomeThings()
{
    Chunk chunk;
    InitChunk(&chunk);

    TValue v;
    v.real = 34.201;
    WriteConstant(&chunk, v, 0);
    WriteConstant(&chunk, v, 1);
    WriteChunk(&chunk, (u8)OpCode::RETURN, 2);

    Debug_DisassembleChunk(&chunk, "test chunk");
    FreeChunk(&chunk);
}
