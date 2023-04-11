#ifndef MEMORY_KHEAP_H_
#define MEMORY_KHEAP_H_
#include "config.h"
#include "stdint.h"
#include "stddef.h"

void k_heap_table_mm_init();
void* k_heap_table_mm_malloc(size_t size);
void k_heap_table_mm_free(void *ptr);

#endif
