#include "config.h"
#include "disk/disk.h"
#include "disk/dstream.h"
#include "drivers/graphic/videomode.h"
#include "fs/pathparser.h"
#include "idt/idt.h"
#include "include/uapi/bootinfo.h"
#include "include/uapi/graphic.h"
#include "io/io.h"
#include "kernel.h"
#include "memory/heapdl.h"
#include "memory/memory.h"
#include "memory/paging/paging.h"
#include "pic/timer.h"
#include "test.h"
#include "util/kutil.h"
#include "util/printf.h"

extern void loadPageDirectory(uint32_t *pd);
/* Kernel Page Directory */
PAGE_DIRECTORY_4KB* kpd = 0;

uint32_t page_directory[1024] __attribute__((aligned(4096)));
uint32_t first_page_table[1024] __attribute__((aligned(4096)));

TIMER *timer = NULL;
TIMER *timer_cursor = NULL;

void pg(void)
{
	//set each entry to not present
	for(int32_t i = 0; i < 1024; i++)
	{
		// This sets the following flags to the pages:
		//   Supervisor: Only kernel-mode can access them
		//   Write Enabled: It can be both read from and written to
		//   Not Present: The page table is not present
		page_directory[i] = 0x00000002;
	}
	// holds the physical address where we want to start mapping these pages to.
	// in this case, we want to map these pages to the very beginning of memory.
	//we will fill all 1024 entries in the table, mapping 4 megabytes
	for(uint32_t i = 0; i < 1024; i++)
	{
	    // As the address is page aligned, it will always leave 12 bits zeroed.
	    // Those bits are used by the attributes ;)
	    first_page_table[i] = (i * 0x1000) | 3; // attributes: supervisor level, read/write, present.
	}
	// attributes: supervisor level, read/write, present
	// page_directory[0] = ((unsigned int)first_page_table) | 3;
	page_directory[0] = ((unsigned int)first_page_table) | 0b111;
	loadPageDirectory(page_directory);
	enable_paging();
}

bool heap_debug()
{
	int32_t i = 0;
	uintptr_t handlers[30000] = {0};
	for (i = 0; i < 28000; i++)
	{
		handlers[i] = (uintptr_t)kzalloc(2048);
		if (!handlers[i])
		{
			printf("NULL, i: %d", i);
			break;
		}

	}
	for (i = 0; i < 28000; i++)
		kfree((void *)handlers[i]);

	char *a = (char *) kzalloc(10);
	printf("a: %p", a);
	printf("f: %d", k_heapdl_mm_get_free()/1024/1024);
	printf("chunks: %d", debug__k_heapdl_mm_get_chunks());
	printf("fc: %d", debug__k_heapdl_mm_get_chunksfree());

	return true;
}


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
	(void) kpd;
	//uint32_t pd_entries_flags = 0b111;
	//kpd = pd_init(pd_entries_flags);
	//paging_switch(kpd);
	//enable_paging();
	// pg();

	printf("Used: %dKB", k_heapdl_mm_get_usage()/1024);
	disk_search_and_init();

	if (!test_all())
	{
		printf("Function test FAIL\n");
	} else
	{
		printf("Function test PASS\n");
	}
	// heap_debug();

	/* Set a timer of 4s */
	timer = timer_alloc();
	timer_settimer(timer, 400, 4);
	timer_cursor = timer_alloc();
	timer_settimer(timer_cursor, 100, 1);
	eventloop();
	// asm("hlt");
}


void eventloop(void)
{
	int32_t usedBytes_keybuf, usedBytes_mousebuf = 0;
	SHEET* sw = get_sheet_window();
	int32_t color = COL8_FFFFFF;
	for(;;)
	{
		if (sw)
		{
			char ctc[40] = {0};
			sprintf(ctc, "%010ld", timer_gettick());
			boxfill8((uintptr_t)sw->buf, sw->bufXsize, COL8_C6C6C6, 40, 28, 119, 43);
			if (fifo8_status_getUsageB(timer->fifo) > 0)
				printf("%d", fifo8_dequeue(timer->fifo));
			if (fifo8_status_getUsageB(timer_cursor->fifo) > 0)
			{
				int32_t timer_dt = fifo8_dequeue(timer_cursor->fifo);
				if (timer_dt == 0)
				{
					timer_settimer(timer_cursor, 100, 1);
					color = COL8_FFFFFF;
				} else
				{
					timer_settimer(timer_cursor, 100, 0);
					color = COL8_C6C6C6;
				}

			}
			/* Cursor */
			boxfill8((uintptr_t)sw->buf, sw->bufXsize, color, 40, 28, 40+7, 28+15);
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


