// simple heap table
#ifndef MEMORY_HEAP_H_
#define MEMORY_HEAP_H_
#include "config.h"
#include "stdint.h"
#include "stddef.h"

#define HEAP_BLOCK_TABLE_ENTRY_TAKEN 0x01
#define HEAP_BLOCK_TABLE_ENTRY_FREE 0x00

#define HEAP_BLOCK_HAS_NEXT 0b10000000
#define HEAP_BLOCK_IS_FIRST  0b01000000

typedef uint8_t HEAP_BLOCK_TABLE_ENTRY; // The address of heap_table

struct heap_table
{
	HEAP_BLOCK_TABLE_ENTRY* entries;
	size_t total_blocks; // total table entries count
};

struct heap
{
	struct heap_table* table;
	void* start_addr;
};


int heap_create(struct heap* heap, void* heap_start, void* heap_end, struct heap_table* table);
void* heap_malloc(struct heap* heap, size_t size);
void heap_free(struct heap* heap, void* ptr);

#endif
