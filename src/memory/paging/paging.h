#ifndef MEMORY_PAGING_PAGING_H_
#define MEMORY_PAGING_PAGING_H_

#include "stdint.h"
#define PAGE_DIRECTORY_ENTRY_4KB_CACHE_DISABLED  0b00010000 // If 0, disable cache
#define PAGE_DIRECTORY_ENTRY_4KB_WRITE_THROUGH   0b00001000 // If 0, enable write back cache instead of write through cache
#define PAGE_DIRECTORY_ENTRY_4KB_ACCESS_FOR_ALL  0b00000100 // If 0, only supervisor can access
#define PAGE_DIRECTORY_ENTRY_4KB_ALLOW_WRITE     0b00000010 // If 0, read only
#define PAGE_DIRECTORY_ENTRY_4KB_IS_PRESENT      0b00000001 // If 0, page is swapped out

#define PAGE_DIRECTORY_TOTAL_ENTRIES             1024
#define PAGE_TABLE_TOTAL_ENTRIES                 1024
#define PAGE_OS_PAGE_SIZE                        4096

struct PAGE_DIRECTORY_ENTRY_4KB
{
	int32_t higher_address : 20; // Bits 31-12 of address, points to a page table
	uint8_t available : 4; // AVL
	uint8_t page_size : 1; // 0 for 4kb entry, 1 for 4MB entry
	uint8_t available2 : 1;
	uint8_t accessed : 1;
	uint8_t cache_disable : 1; // if 0, disable cache
	uint8_t write_through : 1; // if 0, enable write back cache instead of write through cache
	uint8_t access_for_all : 1; // if 0, only supervisor can access
	uint8_t allow_write : 1; // if 0, read only
	uint8_t present_in_physical_memory : 1; // if 0, page is swapped out
}__attribute__((packed));

struct PAGE_DIRECTORY_4KB
{
	struct PAGE_DIRECTORY_ENTRY_4KB* entries;
}__attribute__((packed));

struct PAGE_TABLE_ENTRY_4KB
{
	int32_t higher_address : 20; // Bits 31-12 of address, points to a page table
	uint8_t available : 3; // AVL
	uint8_t global : 1; // G, or 'Global' tells the processor not to invalidate the TLB entry corresponding to the page upon a MOV to CR3 instruction. Bit 7 (PGE) in CR4 must be set to enable global pages.
	uint8_t page_size : 1; // 0 for 4kb entry, 1 for 4MB entry
	uint8_t dirty : 1; // D, or 'Dirty' is used to determine whether a page has been written to.
	uint8_t accessed : 1;
	uint8_t cache_disable : 1; // if 0, disable cache
	uint8_t write_through : 1; // if 0, enable write back cache instead of write through cache
	uint8_t access_for_all : 1; // if 0, only supervisor can access
	uint8_t allow_write : 1; // if 0, read only
	uint8_t present_in_physical_memory : 1; // if 0, page is swapped out
}__attribute__((packed));

typedef struct PAGE_DIRECTORY_ENTRY_4KB_FLAGS
{
	uint8_t available : 4; // AVL
	uint8_t page_size : 1; // 0 for 4kb entry, 1 for 4MB entry
	uint8_t available2 : 1;
	uint8_t accessed : 1;
	uint8_t cache_disable : 1; // if 0, disable cache
	uint8_t write_through : 1; // if 0, enable write back cache instead of write through cache
	uint8_t access_for_all : 1; // if 0, only supervisor can access
	uint8_t allow_write : 1; // if 0, read only
	uint8_t present_in_physical_memory : 1; // if 0, page is swapped out
}__attribute__((packed)) PAGE_DIRECTORY_ENTRY_4KB_FLAGS;
struct PAGE_DIRECTORY_4KB* new_page_table_4KB_4GB(struct PAGE_DIRECTORY_ENTRY_4KB_FLAGS flags);
void paging_switch(struct PAGE_DIRECTORY_4KB* directory);
extern void enable_paging();

#endif
