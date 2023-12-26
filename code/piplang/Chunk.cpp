#include "Chunk.h"

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
