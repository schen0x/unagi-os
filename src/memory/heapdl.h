#ifndef MEMORY_HEAPDL_H_
#define MEMORY_HEAPDL_H_

#include <stdint.h>
#include <stddef.h>
#include "util/dlist.h"
#include "config.h"
/*
 * The HEAP Header.
 * The usable memory is whatever after the header, before the next header, minus the aligment cost.
 *
 * "Chunk's usable mem size" = (CHUNK->all->next) - (CHUNK + sizeof(CHUNK) + 16bit-ALIGNMENT_COST(for start and end))
 *
 * @all 2 pointers, connects the whole HEAP
 * @isUsed true if the chunk is in use
 * @size size of the chunk (This should be tracked because there may exist gap between the current and the next CHUNK)
 * @free 2 pointers, points to the next "CHUNK" (Note: not CHUNK->free but rather CHUNK itself, like CHUNK->all) where isUsed is false.
 *
 * TODO DLIST all, free to circle or not?
 */
typedef struct CHUNK {
    DLIST all;			// 2 pointers, connect the whole HEAP
    bool isUsed;		// Is chunk in use, if not, may try merge chunks
    size_t size;
    DLIST free;
} __attribute__((aligned(OS_MEMORY_ALIGN))) CHUNK; // may be gcc dependent. so that the CHUNK *chunk; (void*) (chunk+1) is always aligned

void k_heapdl_mm_init(uintptr_t mem_start, uintptr_t mem_end);
static void chunk_init(CHUNK *chunk);
static size_t chunk_calc_actual_free(CHUNK *chunk);
static uintptr_t chunk_calc_free_offset(CHUNK *chunk);
static void chunk_engage(CHUNK *chunk);
static void chunk_free(CHUNK *chunk);
static CHUNK* chunk_merge(CHUNK *chunkA, CHUNK *chunkB);
void* k_heapdl_mm_malloc(size_t s);
void k_heapdl_mm_free(void *ptr);
size_t k_heapdl_mm_get_free();

#endif

