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
/* Kernel Page Directory */
static PAGE_DIRECTORY_4KB* kpd = 0;

void kernel_main()
{
	graphic_initialize();
	asm("hlt");
	char msg[] = "A";
	kfprint(msg, 4);
	int32_t x = 10;
	int32_t y = 0;
	y = ++x;
	char n[10] = {0};
	kfprint(hex_to_ascii(n, &y, 4),5); // 0xb 11

	// TODO CACHE OFF && MEMORY TEST
	// TODO e820 routine

	k_mm_init();
	// ==============
	uint32_t pd_entries_flags = 0b111;
	kpd = pd_init(pd_entries_flags);
	paging_switch(kpd);
	enable_paging();

	disk_search_and_init();

	// ==============
	// TODO Try sprintf
	// TODO MOUSE HANDLING
	// TODO TERMINAL

	idt_init();
	asm volatile ("int $0");
	enable_interrupts();
	asm("hlt");
}

