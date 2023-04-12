// memory/memory.h
#ifndef MEMORY_MEMORY_H_
#define MEMORY_MEMORY_H_

#include <stddef.h>

void* kmemset(void* ptr, int c, size_t size);
void* kmemcpy(void* dst, const void* src, size_t size);
void kmemory_init(void *mem, size_t size);
void *k_dl_mm_malloc(size_t size);
void k_dl_mm_free(void *mem);
void* kzalloc(size_t size);
void kfree(void *ptr);

#endif
