#include "memory/paging/paging.h"
#include "memory/memory.h"
#include "paging.h"
#include "status.h"
#include <stdint.h>

extern void paging_load_directory(PAGE_DIRECTORY_ENTRY_4KB* directory);
static struct PAGE_DIRECTORY_4KB* current_directory = 0;

static inline void pd_entry_set_flags(PAGE_DIRECTORY_ENTRY_4KB *e, uint32_t flags)
{
    // e->higher_address	= flags >> 12 & 0xfffff;
    e->AVL0		= flags >> 8 & 0b1111;
    e->PS		= flags >> 7 & 0x1;
    e->AVL1		= flags >> 6 & 0x1;
    e->A		= flags >> 5 & 0x1;
    e->PCD		= flags >> 4 & 0x1;
    e->PWT		= flags >> 3 & 0x1;
    e->U_S		= flags >> 2 & 0x1;
    e->R_W		= flags >> 1 & 0x1;
    e->P		= flags & 0x1;
}

static inline uint32_t pd_entry_to_val(PAGE_DIRECTORY_ENTRY_4KB *e)
{
    return e->higher_address << 12
         | e->AVL0 << 8
         | e->PS << 7
         | e->AVL1 << 6
         | e->A << 5
         | e->PCD << 4
         | e->PWT << 3
         | e->U_S << 2
         | e->R_W << 1
         | e->P;
}

static inline void pd_table_set(PAGE_TABLE_ENTRY_4KB *e, uint32_t data)
{
    e->higher_address	= data >> 12 & 0xfffff;
    e->AVL		= data >> 9 & 0b111;
    e->G		= data >> 8 & 0x1;
    e->PAT		= data >> 7 & 0x1;
    e->D		= data >> 6 & 0x1;
    e->A		= data >> 5 & 0x1;
    e->PCD		= data >> 4 & 0x1;
    e->PWT		= data >> 3 & 0x1;
    e->U_S		= data >> 2 & 0x1;
    e->R_W		= data >> 1 & 0x1;
    e->P		= data & 0x1;
}

/* Create a new Page Directory */
PAGE_DIRECTORY_4KB* pd_init(uint32_t flags)
{
	// 1 block
	PAGE_DIRECTORY_4KB* pd = (PAGE_DIRECTORY_4KB*)kzalloc(sizeof(PAGE_DIRECTORY_4KB));

	// 4Bytes * 1024 = 4KB
	PAGE_DIRECTORY_ENTRY_4KB* pd_entries = (PAGE_DIRECTORY_ENTRY_4KB*)kzalloc(sizeof(PAGE_DIRECTORY_ENTRY_4KB) * PAGE_DIRECTORY_TOTAL_ENTRIES);
	pd->entries = pd_entries;

	uint64_t address_offset = 0; // offset calculated by "table number" and "table entry" number
	// loop through all pd_entries
	for (uint64_t i = 0; i < PAGE_DIRECTORY_TOTAL_ENTRIES; i++)
	{
		// create a table, 4KB
		PAGE_TABLE_ENTRY_4KB* table = (PAGE_TABLE_ENTRY_4KB*)kzalloc(sizeof(PAGE_TABLE_ENTRY_4KB) * PAGE_TABLE_TOTAL_ENTRIES);
		// map to the table
		pd_entries[i].higher_address = (uint32_t) table >> 12 & 0xfffff;
		pd_entry_set_flags(&pd_entries[i], flags);

		// map table to 4GB real address
		for (int64_t j = 0; j < PAGE_TABLE_TOTAL_ENTRIES; j++)
		{
			uint64_t real_mem_address = address_offset;
			address_offset += PAGE_OS_PAGE_SIZE;
			table[j].higher_address = (uint64_t) real_mem_address >> 12 & 0xfffff;
			table[j].U_S = 1;
			table[j].R_W = 1;
			table[j].P = 1;
		}

	}
	return pd;
}

void paging_switch(PAGE_DIRECTORY_4KB* directory)
{
	paging_load_directory(directory->entries);
	current_directory=directory;
}

uint32_t is_address_aligned_4KB(void* addr)
{
	return (uint32_t)addr % PAGE_OS_PAGE_SIZE == 0;
}

/*
 * Spit out the closest lower value that align.
 * Use PAGE_OS_PAGE_SIZE as align.
 * e.g., 4097 -> 4096; 8193 -> 8192;
 */
static uint32_t align_addr_to_lower(uint32_t val)
{
    const uint32_t residual = val % PAGE_OS_PAGE_SIZE;
    if (residual == 0)
    {
	    return val;
    }
	    val -= residual;
    return val;
}

/*
 * Calculate the directory and table index, Given the virtual_address
 * Directory (bits 22-31), Table (bits 12-21), Offset (bits 0-11)
 */
void vaddress_to_page_indexes(void* virtual_address, uint32_t* dir_index_out, uint32_t* table_index_out)
{
	uint32_t virtual_addr_aligned = align_addr_to_lower((uint32_t)virtual_address);
	*dir_index_out = ((uint32_t)virtual_addr_aligned / (PAGE_OS_PAGE_SIZE * PAGE_DIRECTORY_TOTAL_ENTRIES));
	*table_index_out = ((uint32_t)virtual_addr_aligned % (PAGE_OS_PAGE_SIZE * PAGE_DIRECTORY_TOTAL_ENTRIES) / PAGE_OS_PAGE_SIZE);
}

/*
 * Assign a new physical address and flags to the virtual_address.
 * "data" is a page table entry.
 * Example:
 * char* p0 = kzalloc(4096);
 * paging_set_page(kpd->entries , (void *)0x1000, ((uint32_t)p0 & 0xfffff000) | 0b111);
 * enable_paging();
 * // p0 == (char*) 0x1000;
 */
uint32_t paging_set_page(PAGE_DIRECTORY_ENTRY_4KB *dir, void* virtual_address, uint32_t data)
{
	uint32_t dir_index = 0;
	uint32_t table_index = 0;
	vaddress_to_page_indexes(virtual_address, &dir_index, &table_index);
	PAGE_TABLE_ENTRY_4KB *table = (PAGE_TABLE_ENTRY_4KB*)(dir[dir_index].higher_address << 12);
	pd_table_set(&table[table_index], data);
	return 0;
}
