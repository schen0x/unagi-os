#include "kernel.h"
#include "config.h"
#include "util/kutil.h"
#include "idt/idt.h"
#include "io/io.h"
#include "memory/memory.h"
#include "memory/kheap.h"
#include "memory/paging/paging.h"
#include "disk/disk.h"
#include "fs/pathparser.h"
#include "include/uapi/graphic.h"
#include "disk/dstream.h"
#include "include/uapi/bootinfo.h"

/* Kernel Page Directory */
static PAGE_DIRECTORY_4KB* kpd = 0;

void kernel_main()
{
	graphic_initialize((BOOTINFO*) OS_BOOT_BOOTINFO_ADDRESS);

	idt_init();
	_io_sti();
	for (int i = 0 ; i < 100; i++)
	{
		asm volatile ("int $99");
	}

	// TODO CACHE OFF && MEMORY TEST
	// TODO e820 routine

	k_mm_init();
	// ==============
	uint32_t pd_entries_flags = 0b111;
	kpd = pd_init(pd_entries_flags);
	paging_switch(kpd);
	enable_paging();

	disk_search_and_init();

	// asm("hlt");
}


