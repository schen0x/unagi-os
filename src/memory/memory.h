// memory/memory.h
#ifndef MEMORY_MEMORY_H_
#define MEMORY_MEMORY_H_

#include <stddef.h>

void* kmemset(void* ptr, int c, size_t size);
void* kmemcpy(void* dst, const void* src, size_t size);

#endif
