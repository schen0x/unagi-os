#include "config.h"
#include "disk/dstream.h"
#include "drivers/keyboard.h"
#include "drivers/ps2mouse.h"
#include "fs/pathparser.h"
#include "gdt/gdt.h"
#include "include/uapi/bootinfo.h"
#include "io/io.h"
#include "kernel.h"
#include "kernel/process.h"
#include "memory/heapdl.h"
#include "memory/memory.h"
#include "pic/timer.h"
#include "test.h"
#include "util/kutil.h"
#include "util/printf.h"
#include "drivers/ps2kbc.h"
#include "drivers/keyboard.h"
#include "pic/pic.h"
#include "kernel/mprocessfifo.h"

int32_t counter, __GUARD0;
MOUSE_DATA_BUNDLE mouse_one_move = {0};

FIFO32 fifo32_common = {0};
int32_t __fifo32_buffer[4096] = {0};

MPFIFO32 fifoTSS3 = {0}, fifoTSS4 = {0};
int32_t __fifobuf3[4096] = {0}, __fifobuf4[4096] = {0};

TASK *task3, *task4, *taskConsole;
MPFIFO32 *mpfifo32Console = NULL;

uint32_t LRShift = 0;
bool isCapLock = false;
/**
 * bit 0:7, ScrollLock, NumLock, CapsLock, ...
 * So keyLEDs & 4 == isCapsLock
 */
uint8_t keyLEDs = 0;


FIFO32* get_fifo32_common(void)
{
	return &fifo32_common;
}

int32_t get_guard()
{
	return __GUARD0;
}

/**
 * TODO
 *   - e820 routine
 */
void kernel_main(void)
{
	fifo32_init(&fifo32_common, __fifo32_buffer, 4096);
	idt_init();

	/**
	 * PIC_remap() must be later than the idt setup
	 * PIC_remap may be later than the PIC KBD/MOUSE setup
	 * But?
	 *   - Possible QEMU bug
	 *   - More logical to re-wire the int first before enabling/config the chip
	 * But But?
	 *   - QEMU 8.0.0 seems really angry when init PIC before PIC kbd/mouse
	 *   - Yolo, this does not matter, do handling on the KBD and MOUSE init, clear all data
	 */
	PIT_init();
	ps2kbc_KBC_init();
	ps2kbc_MOUSE_init();
	PIC_init(0x20, 0x28);

	graphic_init((BOOTINFO*) OS_BOOT_BOOTINFO_ADDRESS);
	k_mm_init();
	graphic_window_manager_init((BOOTINFO*) OS_BOOT_BOOTINFO_ADDRESS);
	disk_search_and_init();

	_io_sti();
	if (!test_all()) {
		printf("Function test FAIL\n");
	} else {
		printf("Function test PASS\n");
	}

	// heap_debug();

	/* Import GDTR0 and switch to the GDTR1 */
	gdt_migration();

	keyLEDs = (((BOOTINFO*) OS_BOOT_BOOTINFO_ADDRESS)->leds >> 4) & 0b111;

	if (!DEBUG_NO_MULTITASK)
	{
		_io_cli();
		task3 = mprocess_init();
		mprocess_task_run(task3, 1, 2);

		task4 = mprocess_task_alloc();
		/**
		 * -8 if __tss_b_main(...) has 1 parameter (to keep ESP+4 inbound), or
		 * -12 if use a far call; anyway, far jumping is used here.
		 */
		task4->tss.esp = (uint32_t) (uintptr_t) kzalloc(4096 * 16) + 4096*16 - 4;
		task4->tss.eip = (uint32_t) &__tss4_main;
		task4->tss.cs = OS_GDT_KERNEL_CODE_SEGMENT_SELECTOR;
		task4->tss.es = OS_GDT_KERNEL_DATA_SEGMENT_SELECTOR;
		task4->tss.ss = OS_GDT_KERNEL_DATA_SEGMENT_SELECTOR;
		task4->tss.ds = OS_GDT_KERNEL_DATA_SEGMENT_SELECTOR;
		task4->tss.fs = OS_GDT_KERNEL_DATA_SEGMENT_SELECTOR;
		task4->tss.gs = OS_GDT_KERNEL_DATA_SEGMENT_SELECTOR;
		mprocess_task_run(task4, 2, 1);

		taskConsole = mprocess_task_alloc();
		/**
		 * -8 if __tss_b_main(...) has 1 parameter (to keep ESP+4 inbound), or
		 * -12 if use a far call; anyway, far jumping is used here.
		 */
		taskConsole->tss.esp = (uint32_t) (uintptr_t) kzalloc(4096 * 16) + 4096*16 - 8;
		*(uint32_t *)(taskConsole->tss.esp + 4) = (uint32_t) get_sheet_console();
		taskConsole->tss.eip = (uint32_t) &console_main;
		taskConsole->tss.cs = OS_GDT_KERNEL_CODE_SEGMENT_SELECTOR;
		taskConsole->tss.es = OS_GDT_KERNEL_DATA_SEGMENT_SELECTOR;
		taskConsole->tss.ss = OS_GDT_KERNEL_DATA_SEGMENT_SELECTOR;
		taskConsole->tss.ds = OS_GDT_KERNEL_DATA_SEGMENT_SELECTOR;
		taskConsole->tss.fs = OS_GDT_KERNEL_DATA_SEGMENT_SELECTOR;
		taskConsole->tss.gs = OS_GDT_KERNEL_DATA_SEGMENT_SELECTOR;
		mprocess_task_run(taskConsole, 2, 2);
		_io_sti();
	}

	eventloop();
}

/* TSS3 */
void eventloop(void)
{
	int32_t data = 0;
	int32_t data_keymouse = 0;
	int32_t keymousefifobuf_usedBytes = 0;
	MPFIFO32 *keymousefifo = get_keymousefifo();
	keymousefifo->task = task3;

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
		keymousefifobuf_usedBytes = mpfifo32_status_getUsageB(keymousefifo);
		if (!mpfifo32_status_getUsageB(&fifoTSS3) && keymousefifobuf_usedBytes <= 0)
		{
			mprocess_task_sleep(task3);
			_io_sti();
			//asm("pause");
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
		data_keymouse = mpfifo32_dequeue(keymousefifo);
		/* May be -EIO */
		if (data_keymouse < 0)
			// continue;
			goto wait_next_event;

		/**
		 * Keyboard;
		 *   - Get focus (sheet at Top - 1) -> buffer
		 *   - Send scancode to correspond task
		 */
		if (data_keymouse >= DEV_FIFO_KBD_START && data_keymouse < DEV_FIFO_KBD_END)
		{
			int32_t kbdscancode = data_keymouse - DEV_FIFO_KBD_START;
			SHEET *sc = get_sheet_console();
			if (!sc)
			{
fallback:
				int21h_handler(kbdscancode & 0xff);
				continue;
			}
			SHEET *focus = sc->ctl->sheets[sc->ctl->zTop - 1];
			if (focus == sc)
			{
				if (!mpfifo32Console)
					goto fallback;
				mpfifo32_enqueue(mpfifo32Console, data_keymouse);
			} else {
				//printf("d");
			}

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

/* TSS4 */
void __tss4_main()
{
	SHEET* sw = get_sheet_window();
	int32_t data = 0;
	int32_t color = COL8_FFFFFF;

	mpfifo32_init(&fifoTSS4, __fifobuf4, 4096, task4);
	TIMER *timer_render = NULL, *timer_1s = NULL, *timer_5s = NULL;
	timer_1s = timer_alloc_customfifo(&fifoTSS4);
	timer_settimer(timer_1s, 100, 11);
	timer_5s = timer_alloc_customfifo(&fifoTSS4);
	timer_settimer(timer_5s, 500, 15);
	timer_render = timer_alloc_customfifo(&fifoTSS4);
	timer_settimer(timer_render, 50, 16);
	(void) timer_render;
	int32_t counterTSS4 = 0;

	for (;;)
	{
		counterTSS4++;
		_io_cli();

		if (mpfifo32_status_getUsageB(&fifoTSS4) <= 0)
		{
			/* Sleep first, then sti(), to avoid interrupt */
			mprocess_task_sleep(task4);
			_io_sti();
			//asm("pause");

			continue;
		}

		data = mpfifo32_dequeue(&fifoTSS4);

		if (data < 0)
		{
			continue;
		}
		/* Performance Test */
		if (data == 15)
		{
			//printf("s:%d ", (counterTSS4)/5);
			counterTSS4 = 0;
			timer_settimer(timer_5s, 500, 0);
		}

		/* TIMER timer_render, Screen Redraw */
		if (data == 16)
		{
			timer_settimer(timer_render, 8, 0);

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

		if (data == 11)
		{
			timer_settimer(timer_1s, 100, 11);
			if (sw)
			{
				/* Blinking cursor (toggle color) */
				color ^= COL8_C6C6C6;
			}

		}

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
	SCREEN_MOUSEXY xy = {0};
	getMouseXY(&xy);
	/* If the left button is pressed */
	if ((mouse_one_move.btn & 0b1) != 0)
	{
		SHEET *selected = get_sheet_by_cursor(&xy0);
		if (!selected)
			return;
		SHEET *w = get_sheet_window();
		// if (isCursorWithinSheet(&xy0, w))
		SHEET *c = get_sheet_console();
		if (selected == w)
		{
			make_wtitle8((uintptr_t) w->buf, w->bufXsize, w->bufYsize, "window", true);
			make_wtitle8((uintptr_t) c->buf, c->bufXsize, c->bufYsize, "console", false);
			sheet_updown(selected, selected->ctl->zTop - 1);
			sheet_update_sheet(c, 0, 0, c->bufXsize, 21);
			// sheet_slide(c, c->xStart, c->yStart);
		} else if (selected == c) {
			make_wtitle8((uintptr_t) w->buf, w->bufXsize, w->bufYsize, "window", false);
			make_wtitle8((uintptr_t) c->buf, c->bufXsize, c->bufYsize, "console", true);
			sheet_updown(selected, selected->ctl->zTop - 1);
			sheet_update_sheet(w, 0, 0, w->bufXsize, 21);
		} else {
			make_wtitle8((uintptr_t) w->buf, w->bufXsize, w->bufYsize, "window", false);
			make_wtitle8((uintptr_t) c->buf, c->bufXsize, c->bufYsize, "console", false);
			sheet_update_sheet(w, 0, 0, w->bufXsize, 21);
			sheet_update_sheet(c, 0, 0, c->bufXsize, 21);
			return;

		}
		sheet_slide(selected, selected->xStart + xy.mouseX - xy0.mouseX, selected->yStart + xy.mouseY - xy0.mouseY);
	}
	return;
}

void console_main(SHEET *sheet)
{
	TASK *task = mprocess_task_get_current();
	mpfifo32Console = kzalloc(sizeof(MPFIFO32));
	int32_t *__fifobuf = kzalloc(sizeof(int32_t) * 512);
	mpfifo32_init(mpfifo32Console, __fifobuf, 512, task);

	TEXTBOX *t = sheet->textbox;

	int32_t i;
	int32_t cursorColor = COL8_FFFFFF;
	/* Buf position */
	// int32_t cursorX = 8, cursorY = 28;

	uint32_t timeoutBlink = 80;
	TIMER *timer;
	timer = timer_alloc_customfifo(mpfifo32Console);
	timer_settimer(timer, timeoutBlink, 20);
	char c2[10] = {0};

	v_textbox_putfonts8_asc(t, t->charColor, ">");

	for (;;)
	{
		_io_cli();
		if (mpfifo32_status_getUsageB(mpfifo32Console) == 0) {
			mprocess_task_sleep(task);
			_io_sti();
		} else {
			i = mpfifo32_dequeue(mpfifo32Console);
			_io_sti();
			if (i >= DEV_FIFO_KBD_START && i < DEV_FIFO_KBD_END)
			{
				int32_t kbdscancode = i - DEV_FIFO_KBD_START;
				if (kbdscancode == 0x0e)
					// TODO backspace
					continue;
				if (kbdscancode == 0x0f)
					// TODO tab
					continue;
				/* Enter */
				if (kbdscancode == 0x1c)
				{
					// const int32_t prevX = cursorX;
					const int32_t prevY = t->boxY;
					for (int32_t x = t->boxX; x <= t->bufXE - (int32_t)t->incrementX; x++)
					{
						v_textbox_boxfill8(t, t->bgColor, x, prevY, x + t->incrementX, prevY + t->incrementY);
						v_textbox_update_sheet(t->sheet, x, prevY, x + t->incrementX, prevY + t->incrementY);
					}
					uint8_t *lb = t->lineBuf;
					if (t->lineEolPos == 6)
					{
						if (lb[0] == 'w' &&
								lb[1] == 'h' &&
								lb[2] == 'o' &&
								lb[3] == 'a' &&
								lb[4] == 'm' &&
								lb[5] == 'i')
						{
							v_textbox_putfonts8_asc(t, t->charColor, "\n");
							v_textbox_putfonts8_asc(t, COL8_848484, OS_NAME);
						}

					}
					v_textbox_putfonts8_asc(t, t->charColor, "\n");
					v_textbox_putfonts8_asc(t, t->charColor, ">");
					for (int32_t i = 0; i < t->lineEolPos; i++)
					{}
					continue;
				}
				/* LShift */
				if (kbdscancode == 0x2a)
					LRShift |= 1;
				if (kbdscancode == 0x36)
					LRShift |= 2;
				if (kbdscancode == (0x2a | BREAK_MASK))
					LRShift &= ~1;
				if (kbdscancode == (0x36 | BREAK_MASK))
					LRShift &= ~2;
				/* CapsLock */
				if (kbdscancode == (0x3a))
					LRShift ^= 4;

				bool isShift = !!((LRShift & 3) > 0) ^ !!(LRShift & 4);
				char c = input_get_char(kbdscancode, isShift);
				if (!c)
					continue;
				c2[0] = c;
				const int32_t prevX = t->boxX;
				const int32_t prevY = t->boxY;
				v_textbox_linebuf_addchar(t, c);
				v_textbox_boxfill8(t, t->bgColor, prevX, prevY, prevX + t->incrementX, prevY + t->incrementY);
				v_textbox_update_sheet(t->sheet, prevX, prevY, prevX + t->incrementX, prevY + t->incrementY);
				v_textbox_putfonts8_asc(t, t->charColor, c2);
			}
			if (i == 21) {
				timer_settimer(timer, timeoutBlink, 20);
				cursorColor = COL8_FFFFFF;
			}
			if (i == 20) {
				timer_settimer(timer, timeoutBlink, 21);
				cursorColor = COL8_000000;
			}
			v_textbox_boxfill8(t, cursorColor, t->boxX, t->boxY, t->boxX + 7, t->boxY + 15);
			sheet_update_sheet(sheet, t->boxX, t->boxY, t->boxX + 7, t->boxY + 15);
		}
	}

}


