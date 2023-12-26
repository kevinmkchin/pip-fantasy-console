#pragma once

#include "Chunk.h"

void PrintTValue(TValue value);

int DisassembleInstruction(Chunk *chunk, int offset);

void DisassembleChunk(Chunk *chunk, const char *name);

