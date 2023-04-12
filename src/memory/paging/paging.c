#include "memory/paging/paging.h"
#include "memory/memory.h"

extern void paging_load_directory(struct PAGE_DIRECTORY_4KB* directory);
static struct PAGE_DIRECTORY_4KB* current_directory = 0;

struct PAGE_DIRECTORY_4KB* new_page_table_4KB_4GB(struct PAGE_DIRECTORY_ENTRY_4KB_FLAGS flags)
{
	struct PAGE_DIRECTORY_4KB* page_directory = (struct PAGE_DIRECTORY_4KB*)kzalloc(sizeof(struct PAGE_DIRECTORY_4KB));
	struct PAGE_DIRECTORY_ENTRY_4KB* page_directory_entries = (struct PAGE_DIRECTORY_ENTRY_4KB*)kzalloc(sizeof(struct PAGE_DIRECTORY_ENTRY_4KB) * PAGE_DIRECTORY_TOTAL_ENTRIES);
	page_directory->entries = page_directory_entries;
	uint64_t address_offset = 0; // offset calculated by "table number" and "table entry" number
	for (int64_t i = 0; i < PAGE_DIRECTORY_TOTAL_ENTRIES; i++)
	{
		struct PAGE_TABLE_ENTRY_4KB* page_entry = (struct PAGE_TABLE_ENTRY_4KB*)kzalloc(sizeof(struct PAGE_TABLE_ENTRY_4KB) * PAGE_TABLE_TOTAL_ENTRIES);
		page_directory_entries[i].higher_address = (uint32_t) page_entry >> 12;
		page_directory_entries[i].available = flags.available;
		page_directory_entries[i].page_size = 0; // 0 for 4kb entry, 1 for 4MB entry
		page_directory_entries[i].available2 = flags.available2;
		page_directory_entries[i].accessed = flags.accessed;
		page_directory_entries[i].cache_disable = flags.cache_disable;
		page_directory_entries[i].write_through = flags.write_through;
		page_directory_entries[i].access_for_all = flags.access_for_all;
		page_directory_entries[i].allow_write = flags.allow_write;
		page_directory_entries[i].present_in_physical_memory = flags.present_in_physical_memory;
		for (int64_t j = 0; j < PAGE_TABLE_TOTAL_ENTRIES; j++)
		{
			page_entry[j].higher_address = (uint32_t) (address_offset) >> 12;
			page_entry[j].page_size = 0;
			page_entry[j].accessed = flags.accessed;
			page_entry[j].cache_disable = flags.cache_disable;
			page_entry[j].write_through = flags.write_through;
			page_entry[j].access_for_all = flags.access_for_all;
			page_entry[j].allow_write = flags.allow_write;
			page_entry[j].present_in_physical_memory = flags.present_in_physical_memory;

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

