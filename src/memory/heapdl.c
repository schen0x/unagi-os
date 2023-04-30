/* Double Linked List Heap Implementation */
#include "memory/heapdl.h"
#include <stdint.h>
#include <stddef.h>
#include "util/kutil.h"
#include "config.h"

CHUNK *first = NULL, *last = NULL; // Store the head, tail ptr in .data
/*
 * The head of the free CHUNKs
 * NULL if no memory is free.
 */
CHUNK *firstfree = NULL;
static size_t mem_free = 0;

void k_heapdl_mm_init(uintptr_t mem_start, uintptr_t mem_end)
{
	uint8_t *aligned_m_start = (uint8_t *)align_address_to_upper(mem_start, OS_HEAP_BLOCK_SIZE);
	uint8_t *aligned_m_end = (uint8_t *)align_address_to_lower(mem_end, OS_HEAP_ADDRESS);
   	first = (CHUNK*)aligned_m_start;
   	CHUNK *second = first + 1;
   	last = (CHUNK*)aligned_m_end - 1;

   	chunk_init(first);
   	chunk_init(second);
   	chunk_init(last);

	/* Update `DLIST all` */
   	dlist_insert_after(&first->all, &second->all);
   	dlist_insert_after(&second->all, &last->all);

   	/* Make first/last as used so they never get merged */
   	first->isUsed = true;
   	last->isUsed = true;

	/* Update the size for chunk 2 */
	second->size = last - second;

	/* Update the firstfree pointer */
	firstfree = second;
   	mem_free = second->size - align_address_to_upper(sizeof(*second), OS_MEMORY_ALIGN);
}

/*
 * @*chunk must not be NULL and writable
 */
static void chunk_init(CHUNK *chunk)
{
	/* Zero out the HEADER region */
	kmemset(chunk, 0, sizeof(CHUNK));

	chunk->all = chunk->all;
	chunk->isUsed = false;
	chunk->size = align_address_to_upper(sizeof(CHUNK), OS_MEMORY_ALIGN); // Initialize to the size of the header after alignment
	chunk->free = chunk->free;
}

/*
 * @size_t must be aligned
 */
static CHUNK* chunk_slice(CHUNK *chunk, size_t s)
{
	CHUNK *chunkB;
	/* chunkB HEADER address should be at (the original free area + s) */
	chunkB = (CHUNK *) (chunk_calc_free_offset(chunk) + s);
	chunk_init(chunkB);
	chunkB->size = chunk_calc_actual_free(chunk) - s;
	dlist_insert_after(&chunk->all, &chunkB->all);
	dlist_insert_after(&chunk->free, &chunkB->free);

	return chunk;
}

/*
 * Mark a chunk is used
 */
static void chunk_engage(CHUNK *chunk)
{
	chunk->isUsed = true;
	dlist_remove(&chunk->free);
	return;
}

static void chunk_free(CHUNK *chunk)
{
	chunk->isUsed = false;
	// TODO
	return;
}

/*
 * Return the usable free bytes of a chunk, chunk->size - "header_size(aligned)"
 */
static size_t chunk_calc_actual_free(CHUNK *chunk)
{
	/* guaranteed positive in chunk_init */
	return chunk->size - align_address_to_upper(sizeof(CHUNK), OS_MEMORY_ALIGN);
}

/*
 * Return the usable free bytes of a chunk, chunk->size - "header_size(aligned)"
 */
static uintptr_t chunk_calc_free_offset(CHUNK *chunk)
{
	return (uintptr_t)chunk + align_address_to_upper(sizeof(CHUNK), OS_MEMORY_ALIGN);
}


size_t k_heapdl_mm_get_usage()
{
	return mem_free;
}

void* k_heapdl_mm_malloc(size_t s)
{
	if (s > mem_free || !mem_free || !firstfree )
		return NULL;
	/* Iterate all "free" CHUNKs */
	CHUNK *pos;
	DLIST *head = &firstfree->free;
	list_for_each_entry(pos, head, free)
	{
		if (pos->size < s)
			continue;
		size_t volatile chunk_free = chunk_calc_actual_free(pos);
		if (chunk_free < s)
			continue;
		/* If no huge size different, do not fragment the chunk any more, just use it */
		if ((chunk_free - s) < OS_HEAP_BLOCK_SIZE)
		{
			chunk_engage(pos);
			return (void*) chunk_calc_free_offset(pos);
		}
		chunk_slice(pos, s);
		chunk_engage(pos);
		return (void*) chunk_calc_free_offset(pos);
	}
	return NULL;
}

/*
 * The reverse of chunk_calc_free_offset(CHUNK *chunk)
 */
static CHUNK* chunk_calc_chunk_by_ptr(void *ptr)
{
	uintptr_t p = (uintptr_t) ptr;
	p -= align_address_to_upper(sizeof(CHUNK), OS_MEMORY_ALIGN);
	return (CHUNK *) p;
}

/*
 * Idempotent. Can be called with multiple times or with NULL ptr.
 */
void k_heapdl_mm_free(void *ptr)
{
	if (!ptr)
		return;

	CHUNK *chunk = chunk_calc_chunk_by_ptr(ptr);
	if (chunk->isUsed)
		chunk_free(chunk);

	return;
}
