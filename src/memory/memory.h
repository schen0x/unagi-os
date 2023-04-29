// memory/memory.h
#ifndef MEMORY_MEMORY_H_
#define MEMORY_MEMORY_H_

#include <stddef.h>

void kmemory_init(void *mem, size_t size);
void *k_dl_mm_malloc(size_t size);
void k_dl_mm_free(void *mem);
void* kmalloc(size_t size);
void* kzalloc(size_t size);
void kfree(void *ptr);
void k_mm_init();

#endif
