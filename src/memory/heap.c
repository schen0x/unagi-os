// simple heap table
#include "heap.h"
#include "status.h"
#include "memory/memory.h"
#include "util/kutil.h"

static uint32_t HBLOCK_SIZE = OS_HEAP_BLOCK_SIZE; // HEAP TABLE HEAP BLOCK SIZE

static int heap_validate_alignment(void* ptr)
{
	return ((unsigned int)ptr % HBLOCK_SIZE) == 0;
}

/*
 * valid if the given table has enough size for the given memory
 */
static int heap_validate_table(void* heap_start, void* heap_end,  struct heap_table* table)
{
	int res = 0;
	size_t total_memory_size = (size_t)(heap_end - heap_start);
	size_t expected_total_table_blocks = total_memory_size / HBLOCK_SIZE;
	if (table->total_blocks != expected_total_table_blocks)
	{
		return -EINVARG;
	}
	return res;
}

int heap_create(struct heap* heap, void* heap_start, void* heap_end, struct heap_table* table)
{
	int res = 0;
	if (!heap_validate_alignment(heap_start) || !heap_validate_alignment(heap_end))
	{
		res = -EINVARG; // take aligned args only
		goto out; // another error handling method
	}
	res = heap_validate_table(heap_start, heap_end, table);
	if (res < 0)
	{
		return res;
	}

	kmemset(heap, 0, sizeof(struct heap));
	heap->start_addr = heap_start;
	heap->table = table;

	size_t table_size = sizeof(HEAP_BLOCK_TABLE_ENTRY) * table->total_blocks;
	kmemset(table->entries, 0, table_size); // fill the heap table entries with 0
out:
	return 0;
}

/*
 * Spit out the closest upper value that align.
 * Use HBLOCK_SIZE as align.
 * e.g., 50->4096; 4097 -> 8192;
 */
static uint32_t heap_table_heap_align_to_upper(uint32_t val)
{
    volatile uint32_t residual = val % HBLOCK_SIZE;
    if (residual == 0)
    {
	    return val;
    }
	    val += (HBLOCK_SIZE - residual);
    return val;
}

/*
 * Spit out the closest lower value that align.
 * Use HBLOCK_SIZE as align.
 * e.g., 4097 -> 4096; 8193 -> 8192;
 */
static uint32_t heap_table_heap_align_to_lower(uint32_t val)
{
    const uint32_t residual = val % HBLOCK_SIZE;
    if (residual == 0)
    {
	    return val;
    }
	    val -= residual;
    return val;
}

static uint32_t heap_get_entry_type(HEAP_BLOCK_TABLE_ENTRY entry)
{
	return entry & 0x0f; // get the first 4 bits
}

uint32_t heap_get_start_block(struct heap* heap, uint32_t total_blocks)
{
	struct heap_table* table = heap->table;
	uint32_t continuous_free_block_counter = 0;
	uint32_t qualified_start_block = -1;
	for (int64_t i=0; i< (int64_t) table->total_blocks; i++)
	{
		if (heap_get_entry_type(table->entries[i]) != HEAP_BLOCK_TABLE_ENTRY_FREE)
		{
			continuous_free_block_counter = 0; // reset
			continue;
		}
		if (++continuous_free_block_counter == total_blocks)
		{
			qualified_start_block = i;
			return qualified_start_block;
		}
	}
			return -ENOMEM;
}

void* heap_block_to_address(struct heap* heap, uint32_t block)
{
	return heap->start_addr + (block * HBLOCK_SIZE);
}

void heap_mark_blocks_taken(struct heap* heap, uint32_t start_block, uint32_t total_blocks)
{
	uint32_t end_block = start_block + total_blocks - 1;
	HEAP_BLOCK_TABLE_ENTRY entry = HEAP_BLOCK_TABLE_ENTRY_TAKEN;
	if (total_blocks > 1)
	{
		entry |= HEAP_BLOCK_HAS_NEXT;
	}
	// First entry
	heap->table->entries[start_block] = entry | HEAP_BLOCK_IS_FIRST;
	// Middle entries
	for (int64_t i = (int64_t)start_block + 1; i <= (int64_t)end_block - 1; i++)
	{
		heap->table->entries[i] = entry;
	}
	// Last entry
	heap->table->entries[end_block] = HEAP_BLOCK_TABLE_ENTRY_TAKEN;
}

void* heap_malloc_blocks(struct heap* heap, uint32_t total_blocks)
{
	void* address = 0;

	int start_block = heap_get_start_block(heap, total_blocks);
	if (start_block < 0)
	{
		return address; // ERROR
	}
	address = heap_block_to_address(heap, start_block);
	heap_mark_blocks_taken(heap, start_block, total_blocks);
	return address;
}

void* heap_malloc(struct heap* heap, size_t size)
{
	size_t aligned_size = heap_table_heap_align_to_upper(size);
	uint32_t total_blocks = aligned_size / HBLOCK_SIZE;
	return heap_malloc_blocks(heap, total_blocks);
}

int64_t heap_address_to_block(struct heap* heap, void* ptr)
{
	return (ptr - heap->start_addr) / (HBLOCK_SIZE);
}

void heap_mark_blocks_free(struct heap* heap, int64_t start_block)
{
	int64_t i = start_block;
	while(1)
	{
		HEAP_BLOCK_TABLE_ENTRY* current_entry = &heap->table->entries[i];
		if ((*current_entry & 0b1) != HEAP_BLOCK_TABLE_ENTRY_FREE)
		{
			*current_entry = HEAP_BLOCK_TABLE_ENTRY_FREE;
		}
		return;
	}
}


void heap_free(struct heap* heap, void* ptr)
{
	heap_mark_blocks_free(heap, heap_address_to_block(heap, ptr));
}

