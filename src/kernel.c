#include "kernel.h"
#include "config.h"
#include "util/kutil.h"
#include "util/printf.h"
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
	{
		kfprint("Function test FAIL.", 4);
	} else
	{
		kfprint("Function test PASS.", 4);
	}
	uint8_t i0 = 0x28;
	int32_t i1 = 1- ((i0<< 3) & 0x100);
	char c1[20] = {0};
	sprintf(c1, "%4x", i1);
	kfprint(c1, 4);



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
			_io_sti();
			asm("pause");
			continue;
		}
		if (usedBytes_keybuf != 0)
		{
			/*
			 * Because if error returns a int32_t negative number,
			 * when autocasted, the sign digit is NOT discarded by default.
			 * uint8_t scancode = (uint32_t) -1;
			 * -> `scancode` becomes 0xff;
			 */
			int32_t kbdscancode = fifo8_dequeue(&keybuf);
			if (kbdscancode > 0)
				int21h_handler(kbdscancode & 0xff);
		}
		if (usedBytes_mousebuf != 0)
		{
			int32_t mousescancode = fifo8_dequeue(&mousebuf);
			if (mousescancode > 0)
				int2ch_handler(mousescancode & 0xff);

		}
	}

}


