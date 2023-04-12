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

typedef struct PAGE_DIRECTORY_ENTRY_4KB
{
	int32_t higher_address : 20; // Bits 31-12 of address, points to a page table
	uint8_t AVL0 : 4; // Available
	uint8_t PS : 1; // Page Size, 0 for 4kb entry, 1 for 4MB entry
	uint8_t AVL1 : 1;
	uint8_t A : 1; // A, or 'Accessed' is used to discover whether a PDE or PTE was read during virtual address translation. If it has, then the bit is set, otherwise, it is not. Note that, this bit will not be cleared by the CPU, so that burden falls on the OS (if it needs this bit at all).
	uint8_t PCD : 1; // the 'Cache Disable' bit. If the bit is set, the page will not be cached. Otherwise, it will be.
	uint8_t PWT : 1; // controls Write-Through' abilities of the page. If the bit is set, write-through caching is enabled. If not, then write-back is enabled instead.
	uint8_t U_S : 1; // 'User/Supervisor' bit, controls access to the page based on privilege level. If the bit is set, then the page may be accessed by all; if the bit is not set, however, only the supervisor can access it. For a page directory entry, the user bit controls access to all the pages referenced by the page directory entry. Therefore if you wish to make a page a user page, you must set the user bit in the relevant page directory entry as well as the page table entry.
	uint8_t R_W : 1; // 'Read/Write' permissions flag. If the bit is set, the page is read/write. Otherwise when it is not set, the page is read-only. The WP bit in CR0 determines if this is only applied to userland, always giving the kernel write access (the default) or both userland and the kernel (see Intel Manuals 3A 2-20).
	uint8_t P : 1; // 'Present'. If the bit is set, the page is actually in physical memory at the moment. For example, when a page is swapped out, it is not in physical memory and therefore not 'Present'. If a page is called, but not present, a page fault will occur, and the OS should handle it.
}__attribute__((packed)) PAGE_DIRECTORY_ENTRY_4KB;

typedef struct PAGE_DIRECTORY_4KB
{
	struct PAGE_DIRECTORY_ENTRY_4KB* entries;
} PAGE_DIRECTORY_4KB;

typedef struct PAGE_TABLE_ENTRY_4KB
{
	int32_t higher_address : 20; // Bits 31-12 of address, points to a page table
	uint8_t AVL : 3; // AVL
	uint8_t G : 1; // G, or 'Global' tells the processor not to invalidate the TLB entry corresponding to the page upon a MOV to CR3 instruction. Bit 7 (PGE) in CR4 must be set to enable global pages.
	uint8_t PAT : 1; // PAT, or 'Page Attribute Table'. If PAT is supported, then PAT along with PCD and PWT shall indicate the memory caching type. Otherwise, it is reserved and must be set to 0.
	uint8_t D : 1; // D, or 'Dirty' is used to determine whether a page has been written to.
	uint8_t A : 1; // A, or 'Accessed' is used to discover whether a PDE or PTE was read during virtual address translation. If it has, then the bit is set, otherwise, it is not. Note that, this bit will not be cleared by the CPU, so that burden falls on the OS (if it needs this bit at all).
	uint8_t PCD : 1; // PCD, is the 'Cache Disable' bit. If the bit is set, the page will not be cached. Otherwise, it will be.
	uint8_t PWT : 1; // PWT, controls Write-Through' abilities of the page. If the bit is set, write-through caching is enabled. If not, then write-back is enabled instead.
	uint8_t U_S : 1; // the 'User/Supervisor' bit, controls access to the page based on privilege level. If the bit is set, then the page may be accessed by all; if the bit is not set, however, only the supervisor can access it. For a page directory entry, the user bit controls access to all the pages referenced by the page directory entry. Therefore if you wish to make a page a user page, you must set the user bit in the relevant page directory entry as well as the page table entry.
	uint8_t R_W : 1; // the 'Read/Write' permissions flag. If the bit is set, the page is read/write. Otherwise when it is not set, the page is read-only. The WP bit in CR0 determines if this is only applied to userland, always giving the kernel write access (the default) or both userland and the kernel (see Intel Manuals 3A 2-20).
	uint8_t P : 1; // 'Present'. If the bit is set, the page is actually in physical memory at the moment. For example, when a page is swapped out, it is not in physical memory and therefore not 'Present'. If a page is called, but not present, a page fault will occur, and the OS should handle it.
}__attribute__((packed)) PAGE_TABLE_ENTRY_4KB;

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

void paging_switch(PAGE_DIRECTORY_4KB* directory);
extern void enable_paging();
PAGE_DIRECTORY_4KB* pd_init(uint32_t flags);

#endif
