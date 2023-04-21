#include "drivers/graphic/videomode.h"
#include <stdint.h>
#include <stddef.h>
#include "io/io.h"
#define COL8_000000 0 // 0:Black
#define COL8_FF0000 1 // 1:Red+
#define COL8_00FF00 2 // 2:Green+
#define COL8_FFFF00 3 // 3:Yellow+
#define COL8_0000FF 4 // 4:Cyan+
#define COL8_FF00FF 5 // 5:Purple+
#define COL8_00FFFF 6 // 6:Blue+
#define COL8_FFFFFF 7 // 7:White
#define COL8_C6C6C6 8 // 8:Gray+
#define COL8_840000 9 // 9:Red-
#define COL8_008400 10 // 10:Green-
#define COL8_848400 11 // 11:Yellow-
#define COL8_000084 12 // 12:Cyan-
#define COL8_840084 13 // 13:Purple-
#define COL8_008484 14 // 14:Blue-
#define COL8_848484 15 // 15:Gray-
static uintptr_t video_mem_start = (uintptr_t)(0xa0000); /* create a local pointer to the absolute address */
static uintptr_t video_mem_end = (uintptr_t)(0xaffff); /* create a local pointer to the absolute address */

/* Draw stripes on the screen */
static void __draw_stripes()
{
	// use uintptr_t to ensure the size is big enough to hold a pointer
	// cannot do arithmetic operations on (void*) without casting, but can with uintptr_t
	for (uintptr_t i = video_mem_start; i <= video_mem_end; i++)
	{
		// _asm_write_mem8(i, 15); // all white
		// _asm_write_mem8(i, i & 0x0f); // vertical stripes
		*(uint8_t*) i = i & 0x0f;
	}
}

static void __draw_three_boxes()
{
	uintptr_t vram = video_mem_start;
	boxfill8((uint8_t*) vram, 320, COL8_FF0000, 20, 20, 120, 120);
	boxfill8((uint8_t*) vram, 320, COL8_00FF00, 70, 50, 170, 150);
	boxfill8((uint8_t*) vram, 320, COL8_0000FF, 120, 80, 220, 180);
}

static void draw_windows()
{
	int32_t xsize, ysize;
	uint8_t* vram = (uint8_t *) video_mem_start;
	xsize = 320;
	ysize = 200;

	boxfill8(vram, xsize, COL8_008484, 0,		0,		xsize - 1,	ysize - 29); // Blue-, desktop
	boxfill8(vram, xsize, COL8_C6C6C6, 0,		ysize - 28, 	xsize - 1, 	ysize - 28); // Gray, line as shadow, taskbar
	boxfill8(vram, xsize, COL8_FFFFFF, 0,		ysize - 27, 	xsize - 1, 	ysize - 27); // White, line, taskbar
	boxfill8(vram, xsize, COL8_C6C6C6, 0,		ysize - 26, 	xsize - 1, 	ysize - 1); // Gray+, taskbar

	boxfill8(vram, xsize, COL8_FFFFFF, 3,		ysize - 24, 	59, 	ysize - 24); // White, start menu btn.t
	boxfill8(vram, xsize, COL8_FFFFFF, 2,		ysize - 24, 	2, 	ysize - 4); // White, start menu btn.l
	boxfill8(vram, xsize, COL8_848484, 3,		ysize - 4, 	59, 	ysize - 4); // Gray-, start menu btn.b.edge
	boxfill8(vram, xsize, COL8_848484, 59,		ysize - 23, 	59, 	ysize - 5); // Gray-, start menu btn.r.edge
	boxfill8(vram, xsize, COL8_000000, 2,		ysize - 3, 	59, 	ysize - 3); // Black, start menu btn.r.shadow
	boxfill8(vram, xsize, COL8_000000, 60,		ysize - 24, 	60, 	ysize - 3); // Black, start menu btn.r.shadow

	boxfill8(vram, xsize, COL8_848484, xsize - 47,	ysize - 24, xsize - 4, 	ysize - 24); // Gray-, tray.t.edge
	boxfill8(vram, xsize, COL8_848484, xsize - 47,	ysize - 23, xsize - 47,	ysize - 4); // Gray-, tray.l.edge
	boxfill8(vram, xsize, COL8_FFFFFF, xsize - 47,	ysize - 3, xsize - 4,	ysize - 3); // White, tray.b.edge
	boxfill8(vram, xsize, COL8_FFFFFF, xsize - 3,	ysize - 24, xsize - 3,	ysize - 3); // White, tray.r.edge

}

void videomode_window_initialize()
{
	// __draw_stripes();

	init_palette();
	// __draw_three_boxes();
	draw_windows();
}

static void boxfill8(uint8_t* vram, int32_t xsize, uint8_t color, int32_t x0, int32_t y0, int32_t x1, int32_t y1)
{
	for(int32_t y = y0; y <= y1; y++)
	{
		for(int32_t x = x0; x <= x1; x++)
			vram[y * xsize + x] = color;
	}
}

static void init_palette(void)
{
	static unsigned char table_rgb[16 * 3] =
	{
		0x00, 0x00, 0x00, // 0:Black
		0xff, 0x00, 0x00, // 1:Red+
		0x00, 0xff, 0x00, // 2:Green+
		0xff, 0xff, 0x00, // 3:Yellow+
		0x00, 0x00, 0xff, // 4:Cyan+
		0xff, 0x00, 0xff, // 5:Purple+
		0x00, 0xff, 0xff, // 6:Blue+
		0xff, 0xff, 0xff, // 7:White
		0xc6, 0xc6, 0xc6, // 8:Gray+
		0x84, 0x00, 0x00, // 9:Red-
		0x00, 0x84, 0x00, // 10:Green-
		0x84, 0x84, 0x00, // 11:Yellow-
		0x00, 0x00, 0x84, // 12:Cyan-
		0x84, 0x00, 0x84, // 13:Purple-
		0x00, 0x84, 0x84, // 14:Blue-
		0x84, 0x84, 0x84, // 15:Gray-
	};
	set_palette(0, 15, table_rgb);
}

static void set_palette(uint8_t start, uint8_t end, unsigned char *rgb)
{
	uint32_t eflags = _io_get_eflags();
	_io_cli();
	/*
	 * VGA Color Register
	 * DAC Write Address
	 * Writing to this register prepares the DAC hardware
	 * to accept writes of data to the DAC Data Register. The value written
	 * is the index of the first DAC entry to be written (multiple DAC
	 * entries may be written without having to reset the write address due
	 * to the auto-increment.)
	 */
	_io_out8(0x03c8, start);
	for (int32_t i = start; i <= end; i++)
	{
	/*
	 * VGA Color Register
	 * DAC Data
	 * Reading or writing to this register returns a value from the DAC
	 * memory. Three successive I/O operations accesses three intensity
	 * values, first the red, then green, then blue intensity values. The
	 * index of the DAC entry accessed is initially specified by the DAC
	 * Address Read Mode Register or the DAC Address Write Mode Register,
	 * depending on the I/O operation performed. After three I/O operations
	 * the index automatically increments to allow the next DAC entry to be
	 * read without having to reload the index. I/O operations to this port
	 * should always be performed in sets of three, otherwise the results
	 * are dependent on the DAC implementation.
	 */
		_io_out8(0x03c9, rgb[0] / 4);
		_io_out8(0x03c9, rgb[1] / 4);
		_io_out8(0x03c9, rgb[2] / 4);
		rgb += 3;
	}

	_io_set_eflags(eflags);
}

