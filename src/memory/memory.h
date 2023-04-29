// memory/memory.h
#ifndef MEMORY_MEMORY_H_
#define MEMORY_MEMORY_H_

#include <stddef.h>
#include <stdint.h>

void kmemory_init(void *mem, size_t size);
void *k_dl_mm_malloc(size_t size);
void k_dl_mm_free(void *mem);
void* kmalloc(size_t size);
void* kzalloc(size_t size);
void kfree(void *ptr);
void k_mm_init();
uintptr_t kmemtest_subtest(uintptr_t mem_start, uintptr_t mem_end);
uintptr_t kmemtest(uintptr_t mem_start, uintptr_t mem_end);

#endif
