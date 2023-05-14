#include "config.h"
#include "disk/disk.h"
#include "disk/dstream.h"
#include "drivers/graphic/videomode.h"
#include "drivers/keyboard.h"
#include "drivers/ps2mouse.h"
#include "fs/pathparser.h"
#include "gdt/gdt.h"
#include "idt/idt.h"
#include "include/uapi/bootinfo.h"
#include "include/uapi/graphic.h"
#include "io/io.h"
#include "kernel.h"
#include "kernel/process.h"
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
MOUSE_DATA_BUNDLE mouse_one_move = {0};

uint32_t page_directory[1024] __attribute__((aligned(4096)));
uint32_t first_page_table[1024] __attribute__((aligned(4096)));

TIMER *timer0 = NULL;
TIMER *timer_cursor = NULL;
TIMER *timer3 = NULL;

TIMER *timer_ts = NULL;
TIMER *timer_put = NULL;

FIFO32 fifo32_common = {0};
int32_t __fifo32_buffer[4096] = {0};

uint8_t *task_b_esp = NULL;
int32_t counter = 0;

FIFO32* get_fifo32_common(void)
{
	return &fifo32_common;
}

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
	fifo32_init(&fifo32_common, __fifo32_buffer, 4096);
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
	printf("VRAM: %p\n", ((BOOTINFO*) OS_BOOT_BOOTINFO_ADDRESS)->vram);


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

	/* Set a timer of 3s */
	timer0 = timer_alloc();
	timer_settimer(timer0, 300, 0);
	timer_cursor = timer_alloc();
	timer_settimer(timer_cursor, 100, 1);
	timer3 = timer_alloc();
	timer_settimer(timer3, 1000, 3);

	/**
	 * Multi-tasking
	 *   - Import GDTR0 and switch to the GDTR1
	 *   - Setup a TIMER
	 */
	gdt_tss_init();
	task_b_esp = (uint8_t *)kmalloc(4096 * 2);
	__tss_switch4_prep();


	/**
	 * Common TIMERs
	 */
	timer_ts = timer_alloc_customfifobuf(&fifo32_common); // 3, 4
	/* We start at TSS3, this is the first timer to switch to TSS4 */
	// timer_settimer(timer_ts, 10, 4);
	/* Screen Redraw (10ms) */
	timer_put = timer_alloc_customfifobuf(&fifo32_common); // 6
	timer_settimer(timer_put, 300, 6);

	eventloop();
}


/**
 * TSS3
 */
void eventloop(void)
{
	int32_t data = 0;
	int32_t data_keymouse = 0;
	int32_t keymousefifobuf_usedBytes = 0;
	FIFO32 *keymousefifo = get_keymousefifo();
	for(;;)
	{
		counter++;
		/**
		 * Without cli(), it seems the printf in some cases cannot finish (may be buffed sometime)
		 */
		_io_cli();
		/**
		 * Every data in the fifo buffer should be sent by an interrupt
		 * e.g. One mouse move emits 3 data packets, in 3 intterrupts
		 */
		if (!fifo32_status_getUsageB(&fifo32_common))
		{
			goto keymouse;
		}

		data = fifo32_peek(&fifo32_common);

		if (data < 0)
		{
			fifo32_dequeue(&fifo32_common);
			goto wait_next_event;
		}
		/* TIMER timer_ts */
		if (data == 3)
		{
			fifo32_dequeue(&fifo32_common);
			/* Switch to TSS4 after 20ms */
			timer_settimer(timer_ts, 2, 4);
			goto wait_next_event;
		}
		/* TIMER timer_ts */
		if (data == 4)
		{
			process_switch_by_cs_index(data);
			goto wait_next_event;
		}

keymouse:
		 /* Keyboard and Mouse PIC interruptions handling */
		 keymousefifobuf_usedBytes = fifo32_status_getUsageB(keymousefifo);
		 if (keymousefifobuf_usedBytes == 0)
		 {
		 	goto wait_next_event;
		 }
		 data_keymouse = fifo32_dequeue(keymousefifo);
		 /**
		  * Maybe -EIO
		  */
		 if (data_keymouse < 0)
		 	goto wait_next_event;

		 if (data_keymouse >= DEV_FIFO_KBD_START && data_keymouse < DEV_FIFO_KBD_END)
		 {
		 	int32_t kbdscancode = data_keymouse - DEV_FIFO_KBD_START;
		 	int21h_handler(kbdscancode & 0xff);
		 }
		 if (data_keymouse >= DEV_FIFO_MOUSE_START && data_keymouse < DEV_FIFO_MOUSE_END)
		 {
		 	int32_t mousescancode = data_keymouse - DEV_FIFO_MOUSE_START;
		 	int2ch_handler(mousescancode & 0xff);
		 }

wait_next_event:
		_io_sti();
		asm("pause");
	}
}

/*
 * MOUSE data handler
 * scancode: uint8_t _scancode
 */
void int2ch_handler(uint8_t scancode)
{
	ps2mouse_decode(scancode, &mouse_one_move);
	SCREEN_MOUSEXY xy0 = {0};
	getMouseXY(&xy0);
	graphic_move_mouse(&mouse_one_move);
	/* Mutated in the `graphic_move_mouse` */
	SCREEN_MOUSEXY xy1 = {0};
	getMouseXY(&xy1);
	/* If the left button is pressed */
	if ((mouse_one_move.btn & 0b1) != 0)
	{
		SHEET *w = get_sheet_window();
		if (isCursorWithinSheet(&xy1, w))
		{
			sheet_slide(w, w->xStart + xy1.mouseX - xy0.mouseX, w->yStart + xy1.mouseY - xy0.mouseY);
		}

	}
}


void __tss_switch4_prep(void)
{
	TSS32 *tss_b = process_gettssb();
	(void)tss_b;
	tss_b->eip = (uint32_t) &__tss_b_main;
	tss_b->esp = (uint32_t) task_b_esp;
	tss_b->cs = OS_GDT_KERNEL_CODE_SEGMENT_SELECTOR;
	tss_b->es = OS_GDT_KERNEL_DATA_SEGMENT_SELECTOR;
	tss_b->ss = OS_GDT_KERNEL_DATA_SEGMENT_SELECTOR;
	tss_b->fs = OS_GDT_KERNEL_DATA_SEGMENT_SELECTOR;
	tss_b->gs = OS_GDT_KERNEL_DATA_SEGMENT_SELECTOR;
	tss_b->eflags = _io_get_eflags();
	return;
}

/**
 * TSS4
 */
void __tss_b_main(void)
{
	SHEET* sw = get_sheet_window();
	int32_t data = 0;
	int32_t color = COL8_FFFFFF;
	for (;;)
	{
		counter++;
		_io_sti();
		_io_cli();
		if (!fifo32_status_getUsageB(&fifo32_common))
		{
			_io_sti();
			asm("pause");
			continue;
		}

		data = fifo32_peek(&fifo32_common);

		if (data < 0)
		{
			fifo32_dequeue(&fifo32_common);
			continue;
		}
		/* TIMER timer_ts */
		if (data == 3)
		{
			process_switch_by_cs_index(data);
			continue;
		}
		/* TIMER timer_ts */
		if (data == 4)
		{
			fifo32_dequeue(&fifo32_common);
			/* Switch to TSS3 after 20ms */
			timer_settimer(timer_ts, 2, 3);
			continue;
		}

		/* Screen Redraw */
		if (data == 6)
		{
			printf("e46 ");
			fifo32_dequeue(&fifo32_common);
			timer_settimer(timer_put, 10, 6);
			/* Performance test */
			if (fifo32_status_getUsageB(timer0->fifo) > 0)
			{
				fifo32_dequeue(timer0->fifo);
				timer_free(timer0);
				printf("ct10sStart", counter);
				counter = 0;
			}
			if (fifo32_status_getUsageB(timer3->fifo) > 0)
			{
				fifo32_dequeue(timer3->fifo);
				printf("ct10s: %d ", counter);
				timer_settimer(timer3, 1000, 3);
			}
			if (!sw)
				continue;

			char ctc[40] = {0};
			sprintf(ctc, "%010ld", timer_gettick());
			boxfill8((uintptr_t)sw->buf, sw->bufXsize, COL8_C6C6C6, 40, 28, 119, 43);
			if (fifo32_status_getUsageB(timer0->fifo) > 0)
				printf("%d", fifo32_dequeue(timer0->fifo));
			/* Blinking cursor */
			if (fifo32_status_getUsageB(timer_cursor->fifo) > 0)
			{
				int32_t timer_dt = fifo32_dequeue(timer_cursor->fifo);
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
			/* Blinking cursor */
			boxfill8((uintptr_t)sw->buf, sw->bufXsize, color, 40, 28, 40+7, 28+15);
			putfonts8_asc((uintptr_t)sw->buf, sw->bufXsize, 40, 28, COL8_000000, ctc);
			sheet_update_sheet(sw, 40, 28, 120, 44);
		}

	}
}


