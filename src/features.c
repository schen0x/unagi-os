#include "memory/paging/paging.h"
/* Kernel Page Directory */
PAGE_DIRECTORY_4KB* kpd = 0;
uint32_t page_directory[1024] __attribute__((aligned(4096)));
uint32_t first_page_table[1024] __attribute__((aligned(4096)));

void pg(void)
{
	(void) kpd;
	//uint32_t pd_entries_flags = 0b111;
	//kpd = pd_init(pd_entries_flags);
	//paging_switch(kpd);
	//enable_paging();
	//set each entry to not present
	for(int32_t i = 0; i < 1024; i++)
	{
		// This sets the following flags to the pages:
		//   Supervisor: Only kernel-mode can access them
		//   Write Enabled: It can be both read from and written to
		//   Not Present: The page table is not present
		page_directory[i] = 0x00000002;
	}
	// holds the physical address where we want to start mapping these pages to.
	// in this case, we want to map these pages to the very beginning of memory.
	//we will fill all 1024 entries in the table, mapping 4 megabytes
	for(uint32_t i = 0; i < 1024; i++)
	{
	    // As the address is page aligned, it will always leave 12 bits zeroed.
	    // Those bits are used by the attributes ;)
	    first_page_table[i] = (i * 0x1000) | 3; // attributes: supervisor level, read/write, present.
	}
	// attributes: supervisor level, read/write, present
	// page_directory[0] = ((unsigned int)first_page_table) | 3;
	page_directory[0] = ((unsigned int)first_page_table) | 0b111;
	_loadPageDirectory(page_directory);
	enable_paging();
}

