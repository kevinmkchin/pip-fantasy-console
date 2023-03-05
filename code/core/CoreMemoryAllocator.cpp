#include "CoreMemoryAllocator.h"

#include <stdio.h>

// returns pointer aligned forward to given alignment
static u8* PointerAlignForward(u8* ptr, size_t align)
{
	ASSERT((align & (align-1)) == 0) // power of 2	
	size_t p = (size_t)ptr;
	size_t modulo = p & (align-1); // cuz power of 2, faster than modulo
    if(modulo != 0)
    {
        return ptr + (align - modulo);
    }
    return ptr;
}

void MemoryLinearInitialize(MemoryLinearBuffer* buffer, size_t sizeBytes)
{
	buffer->buffer = (u8*) calloc(sizeBytes, 1);
	buffer->bufferSize = sizeBytes;
	buffer->arenaOffset = 0;
}

void* MemoryLinearAllocate(MemoryLinearBuffer* buffer, size_t wantedBytes, size_t align)
{
    u8* current_ptr = buffer->buffer + buffer->arenaOffset;
    u8* aligned_ptr = PointerAlignForward(current_ptr, align);
    size_t offset = aligned_ptr - buffer->buffer;

	if(offset + wantedBytes <= buffer->bufferSize)
	{
		void* ptr = buffer->buffer + offset;
		buffer->arenaOffset = offset + wantedBytes;
		return ptr;
	}
	
	printf("Out of memory in given MemoryLinearBuffer");
	return nullptr;
}



