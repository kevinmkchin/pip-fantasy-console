#pragma once

#include <stdlib.h>

#include "MesaCommon.h"


struct MemoryLinearBuffer
{
	// Linear allocator works best when we don't support freeing memory at the pointer level
	// Carve allocations out of a pre alloced buffer

    // There is no per allocation overhead.
    // The buffer memory is not modified by the allocator.
    // The allocator is not thread-safe.

	u8* buffer = nullptr;
	size_t arenaOffset = 0;
	size_t bufferSize = 0;

};

void MemoryLinearInitialize(MemoryLinearBuffer* buffer, size_t sizeBytes);

void* MemoryLinearAllocate(MemoryLinearBuffer* buffer, size_t wantedBytes, size_t align = 16);

#define MEMORY_LINEAR_ALLOCATE(buffer, type) MemoryLinearAllocate(buffer, sizeof(type), alignof(type))
