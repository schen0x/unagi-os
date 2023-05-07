#include "drivers/graphic/videomode.h"
#include <stdint.h>
#include <stddef.h>
#include "io/io.h"
#include "include/uapi/bootinfo.h"
#include "font/hankaku.h"
#include "util/printf.h"
#include "util/kutil.h"
#include "drivers/graphic/sheet.h"
#include "memory/memory.h"

static uintptr_t video_mem_start = (uintptr_t)(0xa0000); /* create a local pointer to the absolute address */
static uintptr_t video_mem_end = (uintptr_t)(0xaffff); /* create a local pointer to the absolute address */

static const int32_t padding_l = 8;
static int32_t posX = padding_l;
static int32_t posY = 30;
BOOTINFO bibk = {0};
int32_t mouseX, mouseY;

SHTCTL *ctl = NULL;
SHEET *sheet_desktop = NULL;
SHEET *sheet_mouse = NULL;
SHEET *sheet_window = NULL;

void videomode_window_initialize(BOOTINFO* bi)
{
	kmemcpy(&bibk, bi, sizeof(BOOTINFO));
	mouseX = bi->scrnx/2;
	mouseY = bi->scrny/2;

	init_palette();

	/* Write directly to the vram, or use a sheet manager etc. */
	draw_desktop((uintptr_t)bi->vram, bi->scrnx, bi->scrny);
	// putfonts8_asc((uintptr_t)bi->vram, bi->scrnx, 9, 9, COL8_000000, "Haribote OS");
	// putfonts8_asc((uintptr_t)bi->vram, bi->scrnx, 8, 8, COL8_FFFFFF, "Haribote OS");
	// uint8_t mouse[16*16] = {0};
	// init_mouse_cursor8((intptr_t)mouse, COL8_008484);
	// putblock8_8((uintptr_t)bi->vram, bi->scrnx, 16, 16, mouseX, mouseY, mouse, 16);

	/*
	 * Only available after memory manager initialization.
	 * Call later for the sake of modularity.
	 */
	// sheet_initialize((uintptr_t)bi->vram, bi->scrnx, bi->scrny);
	return;
}

/**
 * TODO add comment
 */
SHTCTL* sheet_initialize(uintptr_t vram, int32_t scrnx, int32_t scrny)
{
	ctl = shtctl_init(vram, scrnx, scrny);
	sheet_desktop = sheet_alloc(ctl);
	sheet_mouse = sheet_alloc(ctl);
	uint8_t *buf_desktop = (uint8_t *) kmalloc(scrnx * scrny);
	uint8_t *buf_mouse = (uint8_t *) kmalloc(16 * 16);

	draw_desktop((uintptr_t)buf_desktop, scrnx, scrny);
	putfonts8_asc((uintptr_t)buf_desktop, scrnx, 9, 9, COL8_000000, "Haribote OS");
	putfonts8_asc((uintptr_t)buf_desktop, scrnx, 8, 8, COL8_FFFFFF, "Haribote OS");

	sheet_setbuf(sheet_desktop, buf_desktop, scrnx, scrny, -1);
	/* Set the `color_invisible` of `buf_mouse` buffer to color 99 */
	sheet_setbuf(sheet_mouse, buf_mouse, 16, 16, 99);
	/* Also mouse block bg color to 99 */
	init_mouse_cursor8((intptr_t)buf_mouse, 99);

	/* Set the starting positon of `st_desktop` */
	sheet_slide(sheet_desktop, 0, 0);

	/* Set the starting positon of `st_mouse` */
	int32_t mouseX = (scrnx - 16) / 2;
	int32_t mouseY = (scrny - 16) / 2;
	sheet_slide(sheet_mouse, mouseX, mouseY);


	sheet_window = sheet_alloc(ctl);
	uint8_t *buf_window = (uint8_t *) kmalloc(160 * 68);
	sheet_setbuf(sheet_window, buf_window, 160, 68, -1);
	make_window8((uintptr_t)buf_window, 160, 68, "window");
	sheet_slide(sheet_window, 120, 122);

	sheet_updown(sheet_desktop, 0);
	sheet_updown(sheet_window, 1);
	sheet_updown(sheet_mouse, 2);

	sheet_update_zmap(ctl, 0, 0, scrnx, scrny, 0);
	sheet_update_with_screenxy(ctl, 0, 0, scrnx, scrny, 0, -1);
	return ctl;
}

SHEET* get_sheet_window()
{
	return sheet_window;
}

/*
 * Bitmap of font 'A'
 * 00000000
 * 000**000
 * 000**000
 * 000**000
 * 000**000
 * 00*00*00
 * 00*00*00
 * 00*00*00
 * 00*00*00
 * 0******0
 * 0*0000*0
 * 0*0000*0
 * 0*0000*0
 * ***00***
 */
static uint8_t font_A[16] = {
	0x00, 0x18, 0x18, 0x18, 0x18, 0x24, 0x24, 0x24,
	0x24, 0x7e, 0x42, 0x42, 0x42, 0xe7, 0x00, 0x00
};

/**
 * Put a 16 bytes (8(w) * 16(h)) font to the VRAM
 * In (uint8_t *)vram, each byte is a pixel on the `xsize` * `ysize` screen
 * In 256 color mode, each byte is an index representing a 16-bit color (#FFFFFF), registered in the palette
 */
void putfont8(uintptr_t vram, int32_t xsize, int32_t x, int32_t y, int8_t color, uint8_t *font)
{
	uint8_t row = 0;
	uint8_t *pt = NULL;
	for (int32_t i = 0; i < 16; i++)
	{
		row = font[i];
		pt = (uint8_t *)vram + (y+i) * xsize + x;
		if ((row & 0x80) != 0) { pt[0] = color; }
		if ((row & 0x40) != 0) { pt[1] = color; }
		if ((row & 0x20) != 0) { pt[2] = color; }
		if ((row & 0x10) != 0) { pt[3] = color; }
		if ((row & 0x08) != 0) { pt[4] = color; }
		if ((row & 0x04) != 0) { pt[5] = color; }
		if ((row & 0x02) != 0) { pt[6] = color; }
		if ((row & 0x01) != 0) { pt[7] = color; }
	}

}

static void __write_some_chars(BOOTINFO* bi)
{
	putfont8((uintptr_t)bi->vram, bi->scrnx, 8, 8, COL8_FFFFFF, font_A);
	/* Since "hankaku[4096]" is 0 indexed from '\x00', '\x41' * 16 gives the starting location */
	putfont8((uintptr_t)bi->vram, bi->scrnx, 16, 8, COL8_FFFFFF, hankaku + 'B' * 16);
	putfont8((uintptr_t)bi->vram, bi->scrnx, 24, 8, COL8_FFFFFF, hankaku + 'C' * 16);
	putfont8((uintptr_t)bi->vram, bi->scrnx, 40, 8, COL8_FFFFFF, hankaku + '1' * 16);
	putfont8((uintptr_t)bi->vram, bi->scrnx, 48, 8, COL8_FFFFFF, hankaku + '2' * 16);
	putfont8((uintptr_t)bi->vram, bi->scrnx, 56, 8, COL8_FFFFFF, hankaku + '3' * 16);
}

void putfonts8_asc(uintptr_t vram, int32_t xsize, int32_t x, int32_t y, uint8_t color, char *s)
{
	for(; *s != 0; s++)
	{
		putfont8(vram, xsize, x, y, color, hankaku + *s * 16);
		x += 8;
	}
}

void putfonts8_ascv2(uintptr_t vram, int32_t xsize, int32_t posXnew, int32_t posYnew, uint8_t color, char *s)
{
	posX = posXnew;
	posY = posYnew;
	const int32_t incrementX = 8;
	const int32_t incrementY = 24;
	for(; *s != 0; s++)
	{
		if (*s == '\n')
		{
			posX = padding_l;
			posY += incrementY;
			continue;
		}
		putfont8(vram, xsize, posX, posY, color, hankaku + *s * 16);
		posX += incrementX;
		if (posX > xsize - 16)
		{
			posX = padding_l;
			posY += incrementY;
		}

	}
	return;
}

static void display_scroll(uintptr_t vram, int32_t vga_width, int32_t vga_height)
{
	// TODO Scroll
	// Reset the screen for now
	draw_desktop(vram, vga_width, vga_height);
	posX = padding_l;
	posY = 16;
	uint8_t mouse[16*16] = {0};
	init_mouse_cursor8((intptr_t)mouse, COL8_008484);
	putblock8_8((uintptr_t)bibk.vram, bibk.scrnx, 16, 16, mouseX, mouseY, mouse, 16);
}

void videomode_kfprint(const char* str, uint8_t color)
{
	uint8_t *buf = bibk.vram;
	int32_t canvasWidth = bibk.scrnx;
	int32_t canvasHeight = bibk.scrny;
	if (sheet_desktop != NULL)
	{
		buf = sheet_desktop->buf;
		canvasWidth = bibk.scrnx;
		canvasHeight = bibk.scrny;
	}
	if (!color)
		color = COL8_FFFFFF;

	// if ((int64_t)kstrlen(str) * 8 + posX > (int64_t)((canvasHeight - posY)/24 * canvasWidth))
		// display_scroll((uintptr_t)buf, canvasWidth, canvasHeight);
	putfonts8_ascv2((uintptr_t)buf, canvasWidth, posX, posY, color, (char *) str);
	if (sheet_desktop)
		sheet_update_sheet(sheet_desktop, 0, 0, canvasWidth, canvasHeight);
	return;
}


/*
 * Fill the array
 * int array[1024] = {[0 ... 1023] = COL8_008484}; // same thing with gcc GNU extension
 */
static void init_block_fill(uint8_t *block_start, const uint8_t filling_color, const size_t block_size_in_bytes)
{
	uint8_t *b = (uint8_t*) block_start;
	for (size_t i = 0; i < block_size_in_bytes; i++)
	{
		b[i] = filling_color;
	}
	return;
}

/*
 * Move mouse on the screen based on the offsets
 */
void graphic_move_mouse(MOUSE_DATA_BUNDLE *mouse_one_move)
{
 	if (!ctl)
		return;
	if (!sheet_mouse)
		return;
	uint8_t mouse[16*16] = {0};
	init_mouse_cursor8((intptr_t)mouse, 99);

	int32_t newX = mouseX + mouse_one_move->x;
	int32_t newY = mouseY - mouse_one_move->y;
	if (newX < 0)
		newX = 0;
	if (newX > bibk.scrnx - 1)
		newX = bibk.scrnx - 1;
	if (newY < 0)
		newY = 0;
	if (newY > bibk.scrny - 1)
		newY = bibk.scrny - 1;
	mouseX = newX;
	mouseY = newY;
	// putblock8_8((uintptr_t)bibk.vram, bibk.scrnx, 16, 16, mouseX, mouseY, mouse, 16);
	sheet_slide(sheet_mouse, mouseX, mouseY);
}

/*
 * Parse video_mem ready data for the mouse cursor, store at *(uint8_t*)mouse
 */
void init_mouse_cursor8(intptr_t mouseBuf, uint8_t back_color)
{
	uint8_t *m = (uint8_t *) mouseBuf;
	static char cursor[16][16] = {
		"**************..",
		"*OOOOOOOOOOO*...",
		"*OOOOOOOOOO*....",
		"*OOOOOOOOO*.....",
		"*OOOOOOOO*......",
		"*OOOOOOO*.......",
		"*OOOOOOO*.......",
		"*OOOOOOOO*......",
		"*OOOO**OOO*.....",
		"*OOO*..*OOO*....",
		"*OO*....*OOO*...",
		"*O*......*OOO*..",
		"**........*OOO*.",
		"*..........*OOO*",
		"............*OO*",
		".............***"
	};
	for (int32_t y = 0; y < 16; y++)
	{
		for (int32_t x = 0; x < 16; x++)
		{
			if (cursor[x][y] == '*')
			{
				m[y * 16 + x] = COL8_000000;
				continue;
			}
			if (cursor[y][x] == 'O')
			{
				m[y * 16 + x] = COL8_FFFFFF;
				continue;
			}
			if (cursor[y][x] == '.')
			{
				m[y * 16 + x] = back_color; // keep the original back_color (when hovering)
			}
		}
	}
	return;
}

/*
 * Put *buf (a box) in vram.
 * bxsize must eq pxsize and pysize for now.
 * otherwise OVERFLOW!
 * bxsize: Box xsize (width)
 * FIXME vram, buf, index out of bound.
 * Though for vram, currently probably will not cross 0x000BFFFF so no problem
 *
 */
void putblock8_8(intptr_t vram, int32_t vxsize, int32_t pxsize,
		int32_t pysize, int32_t px0, int32_t py0, uint8_t* buf, int32_t bxsize)
{
	int32_t x, y;
	for (y = 0; y < pysize; y++)
	{
		for (x = 0; x < pxsize; x++)
		{
			((uint8_t *)vram)[(py0 + y) * vxsize + (px0 + x)] = buf[y * bxsize + x];
		}
	}
}

/**
 * Fill the buffer
 * @xsize buffer width
 * @x0 startting coordinate (buffer)
 * @y0 startting coordinate (buffer)
 * @x1 ending coordinate (buffer, included)
 * @y1 ending coordinate (buffer, included)
 */
void boxfill8(uintptr_t vram, int32_t xsize, uint8_t color, int32_t x0, int32_t y0, int32_t x1, int32_t y1)
{
	for(int32_t y = y0; y <= y1; y++)
	{
		for(int32_t x = x0; x <= x1; x++)
			((uint8_t*)vram)[y * xsize + x] = color;
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


/**
 * Draw a notification in the buffer
 * e.g. make_window8(buf_window, 160, 68, "window");
 */
void make_window8(uintptr_t buf, int xsize, int ysize, char *title)
{
    static char closebtn[14][16] = {
        "OOOOOOOOOOOOOOO@",
        "OQQQQQQQQQQQQQ$@",
        "OQQQQQQQQQQQQQ$@",
        "OQQQ@@QQQQ@@QQ$@",
        "OQQQQ@@QQ@@QQQ$@",
        "OQQQQQ@@@@QQQQ$@",
        "OQQQQQQ@@QQQQQ$@",
        "OQQQQQ@@@@QQQQ$@",
        "OQQQQ@@QQ@@QQQ$@",
        "OQQQ@@QQQQ@@QQ$@",
        "OQQQQQQQQQQQQQ$@",
        "OQQQQQQQQQQQQQ$@",
        "O$$$$$$$$$$$$$$@",
        "@@@@@@@@@@@@@@@@"
    };
    int x, y;
    char c;
    boxfill8(buf, xsize, COL8_C6C6C6, 0,         0,         xsize - 1, 0        );
    boxfill8(buf, xsize, COL8_FFFFFF, 1,         1,         xsize - 2, 1        );
    boxfill8(buf, xsize, COL8_C6C6C6, 0,         0,         0,         ysize - 1);
    boxfill8(buf, xsize, COL8_FFFFFF, 1,         1,         1,         ysize - 2);
    boxfill8(buf, xsize, COL8_848484, xsize - 2, 1,         xsize - 2, ysize - 2);
    boxfill8(buf, xsize, COL8_000000, xsize - 1, 0,         xsize - 1, ysize - 1);
    /* The window body */
    boxfill8(buf, xsize, COL8_C6C6C6, 2,         2,         xsize - 3, ysize - 3);
    /* The title bar */
    boxfill8(buf, xsize, COL8_000084, 3,         3,         xsize - 4, 20       );
    boxfill8(buf, xsize, COL8_848484, 1,         ysize - 2, xsize - 2, ysize - 2);
    boxfill8(buf, xsize, COL8_000000, 0,         ysize - 1, xsize - 1, ysize - 1);
    putfonts8_asc(buf, xsize, 24, 4, COL8_FFFFFF, title);
    for (y = 0; y < 14; y++) {
        for (x = 0; x < 16; x++) {
            c = closebtn[y][x];
            if (c == '@') {
		/* Black */
                c = COL8_000000;
            } else if (c == '$') {
		/* Gray - */
                c = COL8_848484;
            } else if (c == 'Q') {
		/* Gray + */
                c = COL8_C6C6C6;
            } else {
		/* White */
                c = COL8_FFFFFF;
            }
	    /* Start pos: (xsize - 21, 5) */
            ((uint8_t *)buf)[(5 + y) * xsize + (xsize - 21 + x)] = c;
        }
    }
    return;
}



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
	boxfill8(vram, 320, COL8_FF0000, 20, 20, 120, 120);
	boxfill8(vram, 320, COL8_00FF00, 70, 50, 170, 150);
	boxfill8(vram, 320, COL8_0000FF, 120, 80, 220, 180);
}

/* Draw the desktop */
static void draw_desktop(uintptr_t vram, int32_t screenXsize, int32_t screenYsize)
{
	int32_t xsize, ysize;
	xsize = screenXsize;
	ysize = screenYsize;

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

