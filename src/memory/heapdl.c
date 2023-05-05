/* Double Linked List Heap Implementation */
#include "memory/heapdl.h"
#include <stdint.h>
#include <stddef.h>
#include "util/kutil.h"
#include "config.h"
#include "util/printf.h"

CHUNK *first = NULL, *last = NULL; // Store the head, tail ptr in .data
/*
 * The head of the free CHUNKs
 * NULL if no memory is free.
 */
CHUNK *free_chunk_head = NULL;
volatile size_t mem_free = 0;

void k_heapdl_mm_init(uintptr_t mem_start, uintptr_t mem_end)
{
	uint8_t *aligned_m_start = (uint8_t *)align_address_to_upper(mem_start, OS_HEAP_BLOCK_SIZE);
	uint8_t *aligned_m_end = (uint8_t *)align_address_to_lower(mem_end, OS_HEAP_BLOCK_SIZE);
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
	second->size = (uintptr_t)last - (uintptr_t)second;

	/* Update the firstfree pointer */
	free_chunk_head = first; // Because head does not loop... So this is always the "first"
				  // The real first free chunk is first->free.next
	dlist_insert_after(&first->free, &second->free);
   	mem_free = second->size - align_address_to_upper(sizeof(CHUNK), OS_MEMORY_ALIGN);
}

/*
 * @*chunk must not be NULL and writable
 */
static void chunk_init(CHUNK *chunk)
{
	/* Zero out the HEADER region */
	kmemset(chunk, 0, sizeof(CHUNK));

	chunk->all.next = &chunk->all;
	chunk->all.prev = &chunk->all;
	chunk->isUsed = false;
	chunk->size = align_address_to_upper(sizeof(CHUNK), OS_MEMORY_ALIGN); // Initialize to the size of the header after alignment
	chunk->free.next = &chunk->free;
	chunk->free.prev = &chunk->free;
}

/*
 * @size_t must be aligned
 */
static CHUNK* chunk_slice(CHUNK *chunk, size_t s)
{
	CHUNK *chunkB;
	/* chunkB HEADER address should be at (the original free area + s) */
	chunkB = (CHUNK *)(((uintptr_t)chunk_calc_free_offset(chunk) + s));
	// printf("chunkB:%p", chunkB);
	// isUsed == false
	chunk_init(chunkB);
	// size
	chunkB->size = chunk_calc_actual_free(chunk) - s;
	chunk->size -= chunkB->size;
	// all, free
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
	mem_free -= chunk->size;
	return;
}


static void chunk_free(CHUNK *chunk)
{
	chunk->isUsed = false;
	mem_free += chunk->size - sizeof(CHUNK);
	CHUNK *prevChunk = container_of(chunk->all.prev, CHUNK, all);
	CHUNK *nextChunk = container_of(chunk->all.next, CHUNK, all);
	if (!(prevChunk->isUsed))
	{
		dlist_insert_after(&prevChunk->free, &chunk->free);
		chunk_merge(prevChunk, chunk);
		return;
	}
	if (!(nextChunk->isUsed))
	{
		dlist_insert_before(&nextChunk->free, &chunk->free);
		chunk_merge(chunk, nextChunk);
		return;
	}

	dlist_insert_after(&free_chunk_head->free, &chunk->free);
	return;
}

/*
 * Merge the two chunks to chunkA.
 */
static CHUNK* chunk_merge(CHUNK *chunkA, CHUNK *chunkB)
{
	dlist_remove(&chunkB->all);
	dlist_remove(&chunkB->free);
	chunkA->size += chunkB->size;
	mem_free += sizeof(CHUNK);
	return chunkA;
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
 * Return the offset where the usable free bytes of a chunk starts, chunk->size - "header_size(aligned)"
 */
static uintptr_t chunk_calc_free_offset(CHUNK *chunk)
{
	return (uintptr_t)chunk + align_address_to_upper(sizeof(CHUNK), OS_MEMORY_ALIGN);
}


size_t k_heapdl_mm_get_free()
{
	return mem_free;
}

void* k_heapdl_mm_malloc(size_t s)
{
	if (s > mem_free || !mem_free || free_chunk_head->free.next == &free_chunk_head->free )
		return NULL;
	/* Iterate all "free" CHUNKs */
	CHUNK *pos;
	DLIST *head = &free_chunk_head->free;
	/*
	 * CHUNK *pos = head->next; &pos->free != head;
	 * Does not loop when array.len == 1
	 */
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
