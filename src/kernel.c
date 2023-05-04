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
#include "drivers/graphic/videomode.h"
#include "test.h"

/* Kernel Page Directory */
PAGE_DIRECTORY_4KB* kpd = 0;

void kernel_main(void)
{
	/* idt */
	idt_init();
	graphic_init((BOOTINFO*) OS_BOOT_BOOTINFO_ADDRESS);

	// uintptr_t mem0 = kmemtest(0x7E00, 0x7ffff);
	uintptr_t mem = kmemtest(OS_HEAP_ADDRESS, 0xbfffffff) / 1024 / 1024; // End at 0x0800_0000 (128MB) in QEMU
	_io_sti();
	// asm("int $99");
	// TODO e820 routine

	k_mm_init();

	graphic_window_manager_init((BOOTINFO*) OS_BOOT_BOOTINFO_ADDRESS);
	// printf("mem_test OK from addr %4x to %4x \n", 0x7E00, mem0);
	printf("mem_test OK from addr %dMB to %dMB \n", OS_HEAP_ADDRESS/1024/1024, mem);


	// ==============
	// (void) kpd;
	// uint32_t pd_entries_flags = 0b111;
	// kpd = pd_init(pd_entries_flags);
	// paging_switch(kpd);
	// enable_paging();

	disk_search_and_init();

	if (!test_all())
	{
		printf("Function test FAIL\n");
	} else
	{
		printf("Function test PASS\n");
	}
	eventloop();
	// asm("hlt");
}

void eventloop(void)
{
	int32_t usedBytes_keybuf, usedBytes_mousebuf = 0;
	SHEET* sw = get_sheet_window();
	int64_t count = 0;
	for(;;)
	{
		if (sw)
		{
			count++;
			char ctc[40] = {0};
			sprintf(ctc, "%010ld", count);
			boxfill8((uintptr_t)sw->buf, sw->bufXsize, COL8_C6C6C6, 40, 28, 119, 43);
			putfonts8_asc((uintptr_t)sw->buf, sw->bufXsize, 40, 28, COL8_000000, ctc);
			sheet_update_sheet(sw, 40, 28, 120, 44);
		}




		/* Keyboard and Mouse PIC interruptions handling */
		_io_cli();
		usedBytes_keybuf = fifo8_status_getUsageB(&keybuf);
		usedBytes_mousebuf = fifo8_status_getUsageB(&mousebuf);
		if (usedBytes_keybuf == 0 && \
			usedBytes_mousebuf == 0)
		{
			goto next0;
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
			if (kbdscancode >= 0)
				int21h_handler(kbdscancode & 0xff);
		}
		if (usedBytes_mousebuf != 0)
		{
			int32_t mousescancode = fifo8_dequeue(&mousebuf);
			if (mousescancode >= 0)
				int2ch_handler(mousescancode & 0xff);

		}
next0:
		_io_sti();
		asm("pause");
		continue;
	}

}


