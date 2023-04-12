#include "memory/paging/paging.h"
#include "memory/memory.h"

extern void paging_load_directory(struct PAGE_DIRECTORY_4KB* directory);
static struct PAGE_DIRECTORY_4KB* current_directory = 0;

struct PAGE_DIRECTORY_4KB* new_page_table_4KB_4GB(struct PAGE_DIRECTORY_ENTRY_4KB_FLAGS flags)
{
	struct PAGE_DIRECTORY_4KB* page_directory = (struct PAGE_DIRECTORY_4KB*)kzalloc(sizeof(struct PAGE_DIRECTORY_ENTRY_4KB) * PAGE_DIRECTORY_TOTAL_ENTRIES);
	int64_t address_offset = 0; // offset calculated by "table number" and "table entry" number
	for (int64_t i = 0; i < PAGE_DIRECTORY_TOTAL_ENTRIES; i++)
	{
		struct PAGE_TABLE_ENTRY_4KB* page_entry = (struct PAGE_TABLE_ENTRY_4KB*)kzalloc(sizeof(struct PAGE_TABLE_ENTRY_4KB) * PAGE_TABLE_TOTAL_ENTRIES);
		page_directory->entries[i].higher_address = (uint32_t) page_entry >> 12;
		page_directory->entries[i].available = flags.available;
		page_directory->entries[i].page_size = 0; // 0 for 4kb entry, 1 for 4MB entry
		page_directory->entries[i].available2 = flags.available2;
		page_directory->entries[i].accessed = flags.accessed;
		page_directory->entries[i].cache_disable = flags.cache_disable;
		page_directory->entries[i].write_through = flags.write_through;
		page_directory->entries[i].access_for_all = flags.access_for_all;
		page_directory->entries[i].present_in_physical_memory = flags.present_in_physical_memory;
		for (int64_t j = 0; j < PAGE_TABLE_TOTAL_ENTRIES; j++)
		{
			page_entry->higher_address = (uint32_t) (address_offset) >> 12;
			page_entry->page_size = 0;
			page_entry->accessed = flags.accessed;
			page_entry->cache_disable = flags.cache_disable;
			page_entry->write_through = flags.write_through;
			page_entry->access_for_all = flags.access_for_all;
			page_entry->present_in_physical_memory = flags.present_in_physical_memory;

			address_offset += PAGE_OS_PAGE_SIZE;
		}

	}
	return page_directory;
}

void paging_switch(struct PAGE_DIRECTORY_4KB* directory)
{
	paging_load_directory(directory);
	current_directory=directory;
}

