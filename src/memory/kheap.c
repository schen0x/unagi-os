#include "memory/kheap.h"
#include "memory/heap.h"
#include "config.h"
#include "include/uapi/graphic.h"
#include "memory/memory.h"
#include <stdint.h>

HEAP kernel_heap = {0};

HEAP_TABLE kernel_heap_table = {0};

/*
 * 1024 * 1024 * 100 = 100 MB heap size
 * 100 MB / 4096 = 25600 block; heap_table_size = <block counts> * entrysize("uint8_t")
 * Put the table in 0x7E00 - 0x7FFFF (480KB), we need 25KB per 100MB
 */
void k_heap_table_mm_init()
{
	int total_memory_blocks = OS_HEAP_SIZE_BYTES / OS_HEAP_BLOCK_SIZE;
	kernel_heap_table.entries = (uint8_t*) OS_HEAP_TABLE_ADDRESS;
	kernel_heap_table.total_blocks = total_memory_blocks;
	void* heap_end = (void*)(OS_HEAP_ADDRESS + OS_HEAP_SIZE_BYTES);
	int res = heap_create(&kernel_heap, (void*)OS_HEAP_ADDRESS, heap_end, &kernel_heap_table);
	if (res < 0)
	{
		kfprint("Fail to allocate heap", 4);
	}
}

void* k_heap_table_mm_malloc(size_t size)
{
	return heap_malloc(&kernel_heap, size);
}

void k_heap_table_mm_free(void *ptr)
{
	heap_free(&kernel_heap, ptr);
}

