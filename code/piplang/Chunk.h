#pragma once

#include "PipLangCommon.h"
#include <vector>


struct Chunk
{
    std::vector<int> *linenumbers; // TODO(Kevin): less memory hogging line number encoding
    std::vector<u8> *bytecode;
    std::vector<TValue> *constants;
};

void InitChunk(Chunk *chunk);
void FreeChunk(Chunk *chunk);
void WriteChunk(Chunk *chunk, u8 byte, int line);
void WriteConstant(Chunk *chunk, TValue value, int line);
