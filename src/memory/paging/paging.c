#include "memory/paging/paging.h"
#include "memory/memory.h"

extern void paging_load_directory(struct PAGE_DIRECTORY_4KB* directory);
static struct PAGE_DIRECTORY_4KB* current_directory = 0;

static inline void pd_entry_set_flags(struct PAGE_DIRECTORY_ENTRY_4KB *e, uint32_t flags)
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

static inline uint32_t pd_entry_to_val(struct PAGE_DIRECTORY_ENTRY_4KB *e)
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
	paging_load_directory(directory);
	current_directory=directory;
}

