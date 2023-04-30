// memory/memory.c
#include "memory/memory.h"
#include "memory/kheap.h"
#include "config.h"
#include "util/kutil.h"
#include "io/io.h"
#include "status.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "memory/heapdl.h"

/*
 * Must check if NULL before using!
 * Return a pointer
 */
void* kzalloc(size_t size)
{

	void* ptr = kmalloc(size);
	if (!ptr)
	{
		return NULL;
	}
	kmemset(ptr, 0, size);
	return ptr;
}

void k_mm_init()
{
	// k_heap_table_mm_init(); // managemend by simple heap table
	k_heapdl_mm_init(OS_HEAP_ADDRESS, OS_HEAP_ADDRESS + OS_HEAP_SIZE_BYTES); // ~128MB
	// k_heapdl_mm_init(0x7c00, 0x7ffff); // ~475KB (aligned), no crash
}

void* kmalloc(size_t size)
{
	// return k_heap_table_mm_malloc(size);
	return k_heapdl_mm_malloc(size);
}


void kfree(void *ptr)
{
	// k_heap_table_mm_free(ptr);
	k_heapdl_mm_free(ptr);
}

/* Memory Test */
uintptr_t kmemtest_subtest(uintptr_t mem_start, uintptr_t mem_end)
{
	uintptr_t addr_current;
	volatile uint32_t *p;
	uint32_t original_data, pat0 = 0xaa55aa55, pat1 = 0x55aa55aa;
	const uint32_t step = 0x1000; // 4KB
	for (addr_current = mem_start; addr_current < mem_end; addr_current += step)
	{
		p = (uint32_t *) addr_current;
		original_data = *p;
		*p = pat0;
		*p = ~*p;
		if (*p != pat1)
		{
// not_memory:
			*p = original_data;
			return addr_current;
		}
		*p = ~*p;
		if (*p != pat0)
		{
			*p = original_data;
			// goto not_memory;
			return addr_current;
		}
		*p = original_data;
	}
	return addr_current - step;
}

/*
 * Memory Test.
 * If 486 or above, the function make sure cache is disabled during the test.
 */
uintptr_t kmemtest(uintptr_t mem_start, uintptr_t mem_end)
{
	bool is_486_or_above = false;
	/* CPU >= 486? */
	const uint32_t eflagsbk = _io_get_eflags();
	uint32_t eflags = eflagsbk;
	eflags |= EFLAGS_MASK_AC;
	_io_set_eflags(eflags);
	eflags = _io_get_eflags();
	/* 386 will auto reset the EFLAGS_MASK_AC bit to 0 */
	if (isMaskBitsAllSet(eflags, EFLAGS_MASK_AC))
		is_486_or_above = true;
	_io_set_eflags(eflagsbk);

	const uint32_t cr0bk = _io_get_cr0();
	if (is_486_or_above)
	{
		/* Disable cache during the memory test */
		_io_set_cr0(cr0bk | CR0_MASK_CD);
	}

	int32_t res = kmemtest_subtest(mem_start, mem_end);

	if (is_486_or_above)
	{
		/* Cleanup, reset the original cr0 flags */
		_io_set_cr0(cr0bk);
	}
	return res;
}


