// memory/memory.c
#include "memory.h"
#include <stddef.h>

void* kmemset(void* ptr, int c, size_t size)
{
	/* write size * c to (*ptr) */
	char* c_ptr = (char*)ptr;
	for (size_t i=0; i < size; i++)
	{
		c_ptr[i] = (char) c;
	}
	return ptr;
}

void* kmemcpy(void* dst, const void* src, size_t size)
{
	char* c_dst = (char*)dst;
	char* c_src = (char*)src;
	for (size_t i=0; i < size; i++)
	{
		c_dst[i] = c_src[i];
	}
	return dst;
}

