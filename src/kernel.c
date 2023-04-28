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

void kernel_main(void)
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

	if (test_kutil() != true || test_fifo8() != true)
		asm("HLT");

	eventloop();
	// asm("hlt");
}

void eventloop(void)
{
	int32_t usedBytes_keybuf, usedBytes_mousebuf = 0;
	for(;;)
	{
		_io_cli();
		usedBytes_keybuf = fifo8_status_getUsageB(&keybuf);
		usedBytes_mousebuf = fifo8_status_getUsageB(&mousebuf);
		if (usedBytes_keybuf == 0 && \
			usedBytes_mousebuf == 0)
		{
			_io_stihlt();
			continue;
		}
		if (usedBytes_keybuf != 0)
		{
			uint8_t kscancode = fifo8_dequeue(&keybuf) & 0xff;
			if (kscancode > 0)
				int21h_handler(kscancode);
		}
		if (usedBytes_mousebuf != 0)
		{
			uint8_t mscancode = fifo8_dequeue(&mousebuf) & 0xff;
			if (mscancode > 0)
				int2ch_handler(mscancode);
		}
	}

}


