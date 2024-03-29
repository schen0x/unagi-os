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
#include "status.h"

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
SHEET *sheet_console = NULL;

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
 * graphic_window_manager_init
 */
SHTCTL* sheet_initialize(uintptr_t vram, int32_t scrnx, int32_t scrny)
{
	/* CTL */
	ctl = shtctl_init(vram, scrnx, scrny);

	/* Desktop */
	sheet_desktop = sheet_alloc(ctl);
	uint8_t *buf_desktop = (uint8_t *) kmalloc(scrnx * scrny);
	draw_desktop((uintptr_t)buf_desktop, scrnx, scrny);
	putfonts8_asc((uintptr_t)buf_desktop, scrnx, 9, 9, COL8_000000, "Haribote OS");
	putfonts8_asc((uintptr_t)buf_desktop, scrnx, 8, 8, COL8_FFFFFF, "Haribote OS");
	sheet_setbuf(sheet_desktop, buf_desktop, scrnx, scrny, -1);
	/* Set the starting positon of `st_desktop` */
	sheet_slide(sheet_desktop, 0, 0);

	/* Mouse */
	sheet_mouse = sheet_alloc(ctl);
	uint8_t *buf_mouse = (uint8_t *) kmalloc(16 * 16);
	/* Set the `color_invisible` of `buf_mouse` buffer to color 99 */
	sheet_setbuf(sheet_mouse, buf_mouse, 16, 16, 99);
	/* Also mouse block bg color to 99 */
	init_mouse_cursor8((intptr_t)buf_mouse, 99);
	/* Set the starting positon of `st_mouse` */
	int32_t mouseX = (scrnx - 16) / 2;
	int32_t mouseY = (scrny - 16) / 2;
	sheet_slide(sheet_mouse, mouseX, mouseY);

	/* Console */
	sheet_console = sheet_alloc(ctl);
	int32_t consoleWidth = 256; // 256
	int32_t consoleHeight = 165; // 165
	uint8_t *buf_console = kzalloc(consoleWidth * consoleHeight);
	sheet_setbuf(sheet_console, buf_console, consoleWidth, consoleHeight, -1);
	make_window8((uintptr_t) buf_console, consoleWidth, consoleHeight, "console", true);
	make_textbox8(sheet_console, 8, 28, consoleWidth - 8*2, consoleHeight - 28 - 9, COL8_000000);
	/* Initialize the textbox according to the make_textbox8 parameters */
	sheet_textbox_alloc(sheet_console, 8, 28, consoleWidth - 8*2, consoleHeight - 28 - 9, COL8_000000, -1, COL8_FFFFFF);

	sheet_slide(sheet_console, 320, 40);

	/* A Floating window */
	sheet_window = sheet_alloc(ctl);
	uint8_t *buf_window = (uint8_t *) kmalloc(160 * 68);
	sheet_setbuf(sheet_window, buf_window, 160, 68, -1);
	make_window8((uintptr_t)buf_window, 160, 68, "window", false);
	sheet_slide(sheet_window, 120, 122);

	sheet_updown(sheet_desktop, 0);
	sheet_updown(sheet_window, 1);
	sheet_updown(sheet_console, 2);
	sheet_updown(sheet_mouse, 3);

	sheet_update_zmap(ctl, 0, 0, scrnx, scrny, 0);
	sheet_update_with_screenxy(ctl, 0, 0, scrnx, scrny, 0, -1);
	return ctl;
}

SHEET* get_sheet_window()
{
	return sheet_window;
}

SHEET* get_sheet_console()
{
	return sheet_console;
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

void v_textbox_putfonts8_asc(TEXTBOX *t, int32_t color, char *s)
{
	if (!t || !t->sheet || !t->sheet->buf)
		return;
	if (color < 0)
		color = t->charColor;
	for(; *s != 0; s++)
	{
		/* Not necessarily the enter key */
		if (*s == '\n')
		{
			/* Clear the current line afterwards */
			v_textbox_boxfill8(t, t->bgColor, t->boxX, t->boxY, t->bufXE, t->boxY + t->incrementY);
			v_textbox_update_sheet(t->sheet, t->boxX, t->boxY, t->bufXE, t->boxY + t->incrementY);
			t->boxX = 0;
			t->boxY += t->incrementY;
			if ((int32_t)(t->boxY + t->bufYS) > t->bufYE)
				t->boxY = 0;
			/* Clear the new line too (we may write whatever into it later) */
			v_textbox_boxfill8(t, t->bgColor, t->boxX, t->boxY, t->bufXE, t->boxY + t->incrementY);
			v_textbox_update_sheet(t->sheet, t->boxX, t->boxY, t->bufXE, t->boxY + t->incrementY);
			continue;
		}
		const int32_t bufX = t->boxX + t->bufXS;
		const int32_t bufY = t->boxY + t->bufYS;
		/* Normal */
		putfont8((uintptr_t)t->sheet->buf, t->sheet->bufXsize, bufX, bufY, t->charColor, hankaku + *s * 16);
		v_textbox_update_sheet(t->sheet, t->boxX, t->boxY, INT32_MAX, t->boxY + t->incrementY);
		t->boxX += t->incrementX;
		if (t->boxX + t->bufXS + (int32_t)t->incrementX > t->bufXE)
		{
			t->boxX = 0;
			t->boxY += t->incrementY;
		}
		if (t->boxY + t->bufYS > t->bufYE)
		{
			t->boxY = 0;
		}
	}
	return;
}

/**
 * Fill a line till the end of line with @fillColor
 */
static void __fill_until_eol(uintptr_t buf, const int32_t bufWidth, const int32_t posX, const int32_t posY, const int32_t lineHeight, const int32_t padding_r, const uint8_t fillColor)
{
	uint8_t *p = NULL;
	uint8_t *eol = NULL;
	for (int32_t row = 0; row < lineHeight; row++)
	{
		p = (uint8_t *)buf + (posY + row) * bufWidth + posX;
		eol = (uint8_t *)buf + (posY + row + 1) * bufWidth - padding_r;
		for (;p < eol; p++)
		{
			*p = fillColor;
		}
	}
	return;
}

/**
 * Call after printing a char; for a buffer with reasonable size, will make sure (x,y) has enough space for at leaset one character
 */
static void __wrap_pos_xy(const int32_t bufWidth, const int32_t bufHeight, int32_t *posX, int32_t *posY, const int32_t fontWidth, const int32_t lineHeight, const int32_t padding_t, const int32_t padding_r, const int32_t padding_b, const int32_t padding_l)
{
	if (*posX + fontWidth > bufWidth - padding_r)
	{
		*posX = padding_l;
		*posY += lineHeight;
	}
	if (*posY + lineHeight > bufHeight - padding_b)
	{
		*posY = padding_t;
	}
}

/**
 * @posX [IN, OUT]
 * @posY [IN, OUT]
 * @pd_* padding top, right, bottom, left
 */
void putfonts8_asc_buf(uintptr_t buf, int32_t bufWidth, int32_t bufHeight, int32_t *posX, int32_t *posY, int32_t padding_t, int32_t padding_r, int32_t padding_b, int32_t padding_l, uint8_t color, char *s)
{
	const int32_t fontWidth = 8;
	const int32_t lineHeight = 24;
	for(; *s != 0; s++)
	{
		if (*s == '\n')
		{
			__fill_until_eol(buf, bufWidth, *posX, *posY, lineHeight, padding_r, COL8_000000);
			//__fill_until_eol(buf, bufWidth, *posX, *posY, lineHeight, padding_r, COL8_840000);
			*posX = padding_l;
			*posY += lineHeight;
			__wrap_pos_xy(bufWidth, bufHeight, posX, posY, fontWidth, lineHeight, padding_t, padding_r, padding_b, padding_l);
			continue;
		}
		putfont8(buf, bufWidth, *posX, *posY, color, hankaku + *s * 16);
		*posX += fontWidth;
		__wrap_pos_xy(bufWidth, bufHeight, posX, posY, fontWidth, lineHeight, padding_t, padding_r, padding_b, padding_l);
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

/**
 * write characters in a buffer
 * @posX relative coordinate in a buffer
 * @posY relative coordinate in a buffer
 */
//void videomode_kfprint_buf(const char* str, uint8_t color, uint8_t *buf, int32_t bufWidth, int32_t bufHeight, int32_t *posX, int32_t *posY)
//{
//	(void) bufHeight;
//	if (!color)
//		color = COL8_FFFFFF;
//
//	// if ((int64_t)kstrlen(str) * 8 + posX > (int64_t)((canvasHeight - posY)/24 * canvasWidth))
//		// display_scroll((uintptr_t)buf, canvasWidth, canvasHeight);
//	putfonts8_asc_buf((uintptr_t)buf, bufWidth, bufHeight, posX, posY, color, (char *) str);
//	return;
//}

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
 * 0 based xy
 */
int32_t __v_textbox_xy_to_sheetxy(TEXTBOX *t, const int32_t textboxX, const int32_t textboxY, int32_t *sheetX, int32_t *sheetY)
{
	if (!t->sheet || !t->sheet->buf)
		return -EIO;
	int64_t sX = (int64_t) textboxX + (int64_t) t->bufXS;
	int64_t sY = (int64_t) textboxY + (int64_t) t->bufYS;

	if (sX >= t->bufXE)
		*sheetX = t->bufXE;
	else
		*sheetX = sX;
	if (sY >= t->bufYE)
		*sheetY = t->bufYE;
	else
		*sheetY = sY;
	return 0;
}
/**
 * use relative xy coordinate in the textbox
 * x0y0 x1y1 are inclusive
 * TODO guard overflow
 */
void v_textbox_boxfill8(TEXTBOX *t, uint8_t color, int32_t x0, int32_t y0, int32_t x1, int32_t y1)
{
	if (!t->sheet || !t->sheet->buf)
		return;
	int32_t sheetX0 = 0;
	int32_t sheetY0 = 0;
	int32_t sheetX1 = 0;
	int32_t sheetY1 = 0;
	int32_t r0 = __v_textbox_xy_to_sheetxy(t, x0, y0, &sheetX0, &sheetY0);
	int32_t r1 = __v_textbox_xy_to_sheetxy(t, x1, y1, &sheetX1, &sheetY1);
	if (r0 < 0 || r1 < 0)
		return;

	for(int32_t y = sheetY0; y <= sheetY1; y++)
	{
		for(int32_t x = sheetX0; x <= sheetX1; x++)
			((uint8_t*)t->sheet->buf)[y * t->sheet->bufXsize + x] = color;
	}
	return;
}

void v_textbox_reset(TEXTBOX *t)
{
	if (!t || !t->lineBuf)
		return;
	v_textbox_boxfill8(t, t->bgColor, 0, 0, t->bufXE - t->bufXS, t->bufYE - t->bufYS);
	v_textbox_update_sheet(t->sheet, 0, 0, t->bufXE - t->bufXS, t->bufYE - t->bufYS);
	t->boxX = 0;
	t->boxY = 0;
	v_textbox_linebuf_clear(t);
	return;
}


/**
 * TODO probably use DLIST to implement the linebuffer
 */
int32_t v_textbox_linebuf_addchar(TEXTBOX *t, const uint8_t c)
{
	if (!t || !t->lineBuf)
		return -EIO;
	if (t->lineEolPos >= OS_TEXTBOX_LINE_BUFFER_SIZE - 1)
		return -EIO;
	if (t->lineCharPos < t->lineEolPos)
	{
		for (int32_t i = t->lineEolPos; i >= t->lineCharPos; i--)
		{
			t->lineBuf[i+1] = t->lineBuf[i];
		}
	}
	t->lineBuf[t->lineCharPos++] = c;
	t->lineEolPos++;
	return 0;
}

void v_textbox_linebuf_clear(TEXTBOX *t)
{
	if (!t || !t->lineBuf)
		return;
	for (int32_t i = 0; i < t->lineEolPos; i++)
	{
		t->lineBuf[i] = 0;
	}
	t->lineCharPos = 0;
	t->lineEolPos = 0;
	return;
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
void make_window8(uintptr_t buf, int xsize, int ysize, char *title, bool isFocus)
{
    boxfill8(buf, xsize, COL8_C6C6C6, 0,         0,         xsize - 1, 0        );
    boxfill8(buf, xsize, COL8_FFFFFF, 1,         1,         xsize - 2, 1        );
    boxfill8(buf, xsize, COL8_C6C6C6, 0,         0,         0,         ysize - 1);
    boxfill8(buf, xsize, COL8_FFFFFF, 1,         1,         1,         ysize - 2);
    boxfill8(buf, xsize, COL8_848484, xsize - 2, 1,         xsize - 2, ysize - 2);
    boxfill8(buf, xsize, COL8_000000, xsize - 1, 0,         xsize - 1, ysize - 1);
    /* The window body */
    boxfill8(buf, xsize, COL8_C6C6C6, 2,         2,         xsize - 3, ysize - 3);
    boxfill8(buf, xsize, COL8_848484, 1,         ysize - 2, xsize - 2, ysize - 2);
    boxfill8(buf, xsize, COL8_000000, 0,         ysize - 1, xsize - 1, ysize - 1);
    make_wtitle8(buf, xsize, ysize, title, isFocus);
    return;
}


void make_wtitle8(uintptr_t buf, int xsize, int ysize, char *title, bool isFocus)
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
    char c, titleFontColor, titleBarColor;
    (void) ysize; // TODO check if in bound

    if (isFocus)
    {
	    titleFontColor = COL8_FFFFFF;
	    titleBarColor = COL8_000084;
    } else {
	    titleFontColor = COL8_C6C6C6;
	    titleBarColor = COL8_848484;
    }

    /* The title bar */
    boxfill8(buf, xsize, titleBarColor, 3,         3,         xsize - 4, 20       );
    putfonts8_asc(buf, xsize, 24, 4, titleFontColor, title);
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

}

SCREEN_MOUSEXY* getMouseXY(SCREEN_MOUSEXY *xy)
{
	xy->mouseX = mouseX;
	xy->mouseY = mouseY;
	return xy;
}

bool isCursorWithinSheet(const SCREEN_MOUSEXY *xy, const SHEET *s)
{
	if (!xy || !s)
		return false;
	if ((xy->mouseX >= s->xStart) &&\
			(xy->mouseX - s->bufXsize <= s->xStart) &&\
			(xy->mouseY >= s->yStart) &&\
			(xy->mouseY - s->bufYsize <= s->yStart))
		return true;
	return false;
}

/* Loop sheets from top to down, find the intuitively proper SHEET where the mouse is pointing at */
SHEET* get_sheet_by_cursor(const SCREEN_MOUSEXY *xy)
{
	if (!xy)
		return NULL;
	/* zTop is mouse; 0 is desktop */
	for (int32_t i = ctl->zTop - 1; i >= 0; i--)
	{

		SHEET *s = ctl->sheets[i];
		if (isCursorWithinSheet(xy, s))
			return s;
	}
	return NULL;
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

/* Write a box in a given buffer */
void make_textbox8(SHEET *sheet, int32_t xStart, int32_t yStart, int32_t width, int32_t height, int8_t color)
{

	int32_t xEnd = xStart + width;
	int32_t yEnd = yStart + height;
	boxfill8((uintptr_t) sheet->buf, sheet->bufXsize, COL8_848484, xStart - 2, yStart - 3, xEnd + 1, yStart - 3);
	boxfill8((uintptr_t) sheet->buf, sheet->bufXsize, COL8_848484, xStart - 3, yStart - 3, xEnd - 3, yEnd + 1);

	boxfill8((uintptr_t) sheet->buf, sheet->bufXsize, COL8_FFFFFF, xStart - 3, yEnd + 2, xEnd + 1, yEnd + 2);
	boxfill8((uintptr_t) sheet->buf, sheet->bufXsize, COL8_FFFFFF, xStart + 2, yStart - 3, xStart + 1, yStart + 1);
	boxfill8((uintptr_t) sheet->buf, sheet->bufXsize, COL8_000000, xStart - 1, yStart - 2, xEnd + 0, yStart - 2);
	boxfill8((uintptr_t) sheet->buf, sheet->bufXsize, COL8_000000, xStart - 2, yStart - 2, xStart - 2, yEnd + 0);
	boxfill8((uintptr_t) sheet->buf, sheet->bufXsize, COL8_C6C6C6, xStart - 2, yEnd + 1, xEnd + 0, yEnd + 1);
	boxfill8((uintptr_t) sheet->buf, sheet->bufXsize, COL8_C6C6C6, xEnd + 1, yStart - 2, xEnd + 1, yEnd + 1);
	boxfill8((uintptr_t) sheet->buf, sheet->bufXsize, color,           xStart - 1, yStart - 1, xEnd + 0, yEnd + 0);
}
