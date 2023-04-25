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
	idt_init();
	graphic_initialize((BOOTINFO*) OS_BOOT_BOOTINFO_ADDRESS);
	_io_sti();
	asm("int $99");
	// TODO CACHE OFF && MEMORY TEST
	// TODO e820 routine

	k_mm_init();
	// ==============
	uint32_t pd_entries_flags = 0b111;
	kpd = pd_init(pd_entries_flags);
	paging_switch(kpd);
	enable_paging();

	disk_search_and_init();


	for(;;)
	{
		_io_cli();
		if(keybuf.len == 0)
		{
			_io_stihlt();
		}
		int32_t i = keybuf.data[keybuf.next_r];
		keybuf.len --;
		keybuf.next_r++;
		if (keybuf.next_r == 32)
		{
			keybuf.next_r = 0;
		}
		_io_sti();
		int21h_handler(i);
	}
}


