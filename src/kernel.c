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

FIFO32 fifo32_common = {0};
int32_t __fifo32_buffer[4096] = {0};

int32_t counter = 0;

MPFIFO32 fifoTSS3 = {0};
int32_t __fifobuf3[4096] = {0};
MPFIFO32 fifoTSS4 = {0};
int32_t __fifobuf4[4096] = {0};

TASK *task3;
TASK *task4;

int32_t GUARD;

FIFO32* get_fifo32_common(void)
{
	return &fifo32_common;
}

int32_t get_guard()
{
	return GUARD;
}

void pg(void)
{
	(void) kpd;
	//uint32_t pd_entries_flags = 0b111;
	//kpd = pd_init(pd_entries_flags);
	//paging_switch(kpd);
	//enable_paging();
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
	idt_init();
	graphic_init((BOOTINFO*) OS_BOOT_BOOTINFO_ADDRESS);
	// uintptr_t mem0 = kmemtest(0x7E00, 0x7ffff);
	uintptr_t mem = kmemtest(OS_HEAP_ADDRESS, 0xbfffffff) / 1024 / 1024; // End at 0x0800_0000 (128MB) in QEMU
	_io_sti();
	// TODO e820 routine

	k_mm_init();

	graphic_window_manager_init((BOOTINFO*) OS_BOOT_BOOTINFO_ADDRESS);
	// printf("mem_test OK from addr %4x to %4x \n", 0x7E00, mem0);
	printf("mem_test OK,&=%dMB~%dMB\n", OS_HEAP_ADDRESS/1024/1024, mem);
	// printf("VRAM: %p\n", ((BOOTINFO*) OS_BOOT_BOOTINFO_ADDRESS)->vram);
	// pg();
	// printf("Used: %dKB", k_heapdl_mm_get_usage()/1024);
	disk_search_and_init();

	if (!test_all())
	{
		printf("Function test FAIL\n");
	} else
	{
		printf("Function test PASS\n");
	}
	// heap_debug();

	/**
	 *  Import GDTR0 and switch to the GDTR1
	 */
	gdt_migration();

	task3 = mprocess_init();
	task4 = mprocess_task_alloc();
//	GUARD = 1;

	/**
	 * -8 if __tss_b_main(...) has 1 parameter (to keep ESP+4 inbound), or
	 * -12 if use a far call; anyway, far jump is used here.
	 */
	task4->tss.esp = (uint32_t) (uintptr_t) kmalloc(4096 * 16) + 4096*16 - 4;
	task4->tss.eip = (uint32_t) &__tss_b_main;
	task4->tss.cs = OS_GDT_KERNEL_CODE_SEGMENT_SELECTOR;
	task4->tss.es = OS_GDT_KERNEL_DATA_SEGMENT_SELECTOR;
	task4->tss.ss = OS_GDT_KERNEL_DATA_SEGMENT_SELECTOR;
	task4->tss.ds = OS_GDT_KERNEL_DATA_SEGMENT_SELECTOR;
	task4->tss.fs = OS_GDT_KERNEL_DATA_SEGMENT_SELECTOR;
	task4->tss.gs = OS_GDT_KERNEL_DATA_SEGMENT_SELECTOR;
	mprocess_task_run(task4);
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

	// FIFO32 fifoTSS3 = {0};
	// int32_t __fifobuf[4096] = {0};

	mpfifo32_init(&fifoTSS3, __fifobuf3, 4096, task3);
	TIMER *timer_put = NULL, *timer_1s = NULL;
	(void) timer_put;

	timer_1s = timer_alloc_customfifo(&fifoTSS3);
	timer_settimer(timer_1s, 100, 1);

	int32_t countTSS3 = 0;

	for(;;)
	{
		countTSS3++;
		/**
		 * Without cli(), it seems the printf in some cases cannot finish (may be buffed sometime)
		 */
		_io_cli();
		keymousefifobuf_usedBytes = fifo32_status_getUsageB(keymousefifo);
		if (!mpfifo32_status_getUsageB(&fifoTSS3) && keymousefifobuf_usedBytes <= 0)
		{
			_io_sti();
			asm("pause");
			continue;
		}
		/**
		 * Every data in the fifo buffer should be sent by an interrupt
		 * e.g. One mouse move emits 3 data packets, in 3 intterrupts
		 */
		if (!mpfifo32_status_getUsageB(&fifoTSS3))
		{
			goto keymouse;
			// goto wait_next_event;
		}

		data = mpfifo32_dequeue(&fifoTSS3);

		if (data < 0)
		{
			goto wait_next_event;
		}

		if (data == 1)
		{
			timer_settimer(timer_1s, 100, 1);
		}

		/* TIMER timer_render */
		if (data == 6)
		{
			// timer_settimer(timer_render, 10, 6);
		}

keymouse:
		/* Keyboard and Mouse PIC interruptions handling */
		if (keymousefifobuf_usedBytes == 0)
		{
			goto wait_next_event;
		}
		data_keymouse = fifo32_dequeue(keymousefifo);
		/**
		 * Maybe -EIO
		 */
		if (data_keymouse < 0)
			// continue;
			goto wait_next_event;

		if (data_keymouse >= DEV_FIFO_KBD_START && data_keymouse < DEV_FIFO_KBD_END)
		{
			int32_t kbdscancode = data_keymouse - DEV_FIFO_KBD_START;
			int21h_handler(kbdscancode & 0xff);
		} else if (data_keymouse >= DEV_FIFO_MOUSE_START && data_keymouse < DEV_FIFO_MOUSE_END)
		{
			int32_t mousescancode = data_keymouse - DEV_FIFO_MOUSE_START;
			int2ch_handler(mousescancode & 0xff);
		}

wait_next_event:
		_io_sti();
		/* Somewhere (e.g. here or in the first "if" block), a pause is necessary. Because without a pause, the loop will be infinite _cli() forever */
		// asm("pause");
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


/**
 * TSS4
 */
// void __tss_b_main(SHEET* sw)
void __tss_b_main()
{
	SHEET* sw = get_sheet_window();
	int32_t data = 0;
	int32_t color = COL8_FFFFFF;

	mpfifo32_init(&fifoTSS4, __fifobuf4, 4096, task4);
	TIMER *timer_render = NULL, *timer_1s = NULL, *timer_5s = NULL;
	timer_1s = timer_alloc_customfifo(&fifoTSS4);
	timer_settimer(timer_1s, 100, 1);
	timer_5s = timer_alloc_customfifo(&fifoTSS4);
	timer_settimer(timer_5s, 500, 5);
	timer_render = timer_alloc_customfifo(&fifoTSS4);
	timer_settimer(timer_render, 10, 6);
	(void) timer_render;
	int32_t counterTSS4 = 0;

	for (;;)
	{
		counterTSS4++;
		_io_cli();

		if (mpfifo32_status_getUsageB(&fifoTSS4) <= 0)
		{
			_io_sti();
			// mprocess_task_sleep(task4);
			asm("pause");
			continue;
		}

		data = mpfifo32_dequeue(&fifoTSS4);

		if (data == 1)
		{
			timer_settimer(timer_1s, 100, 1);
		}

		if (data < 0)
		{
			continue;
		}
		/* Performance Test */
		if (data == 5)
		{
			//printf("s:%d ", (counterTSS4)/5);
			counterTSS4 = 0;
			timer_settimer(timer_5s, 500, 0);
		}

		/* TIMER timer_render, Screen Redraw */
		if (data == 6)
		{
			timer_settimer(timer_render, 10, 0);

			if (sw)
			{
				char ctc[40] = {0};
				sprintf(ctc, "%010ld", timer_gettick());
				boxfill8((uintptr_t)sw->buf, sw->bufXsize, COL8_C6C6C6, 40, 28, 119, 43);
				boxfill8((uintptr_t)sw->buf, sw->bufXsize, color, 40, 28, 40+7, 28+15);
				putfonts8_asc((uintptr_t)sw->buf, sw->bufXsize, 40, 28, COL8_000000, ctc);
				sheet_update_sheet(sw, 40, 28, 120, 44);

			}

		}

		if (data == 1)
		{
			timer_settimer(timer_1s, 100, 1);
			if (sw)
			{
				/* Blinking cursor (toggle color) */
				color ^= COL8_C6C6C6;
			}

		}

	}
}


