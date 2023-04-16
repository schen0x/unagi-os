#include <stdint.h>
#include <stddef.h>
#include "config.h"
#include "drivers/graphic/colortextmode.h"
#include "util/kutil.h"
static uint16_t* video_mem = (uint16_t*)(0xB8000); /* create a local pointer to the absolute address */
/* TODO multi thread handling */
static uint16_t current_terminal_row = 0; // y
static uint16_t current_terminal_col = 0; // x

/* Color Text Mode init */
void colortextmode_terminal_initialize()
{
	for (int64_t y = 0; y < VGA_HEIGHT; y++)
	{
		for (int64_t x = 0; x < VGA_WIDTH; x++ )
		{
			colortextmode_terminal_put_char(x, y, ' ', 0); // Fill screen with black
		}
	}
}


static uint16_t colortextmode_terminal_make_char(const char c, const uint8_t color)
{
	/* text mode, litten endian */
	return (color << 8) | c;
}

void colortextmode_terminal_put_char(const int64_t x, const int64_t y, const char c, const uint8_t color)
{
	/* Put char at x,y */

	video_mem[(y * VGA_WIDTH) + x] = colortextmode_terminal_make_char(c, color);
}

static void _terminal_scroll()
{
	// Move all lines to previous line
	for (size_t y = 1; y < VGA_HEIGHT; y++)
	{
		for (size_t x = 0; x < VGA_WIDTH; x++)
		{
			video_mem[((y - 1) * VGA_WIDTH) + x] = video_mem[(y * VGA_WIDTH) + x];
		}
	}

	// Clear the last row
	for (size_t z = 0; z < VGA_WIDTH; z++)
	{
		colortextmode_terminal_put_char(z, VGA_HEIGHT - 1, ' ', 0);
	}
}

static void current_terminal_row_scrolldown()
{
	current_terminal_row += 1;
	if (current_terminal_row >= VGA_HEIGHT)
	{
		_terminal_scroll();
		current_terminal_row = VGA_HEIGHT - 1;
	}
}

static void terminal_write_char(const char c, const uint8_t color)
{
	if (c == '\n')
	{
		current_terminal_col = 0;
		current_terminal_row_scrolldown();
		return;
	}

	colortextmode_terminal_put_char(current_terminal_col, current_terminal_row, c, color);
	current_terminal_col += 1;
	if (current_terminal_col >= VGA_WIDTH)
	{
		current_terminal_col = 0;
		current_terminal_row_scrolldown();
	}
}

void colortextmode_kprint(const char* str, const size_t len, const uint8_t color)
{
	char error_msg[] = "kernel.c/colortextmode_kprint: Input too long";
	if (len > 51200)
	{
		for (size_t i = 0; i < len; i++)
		{
			terminal_write_char(error_msg[i], 4);
		}
		return;

	}
	for (size_t i = 0; i < len; i++)
	{
		terminal_write_char(str[i], color);
	}
}

void colortextmode_kfprint(const char* str, const uint8_t color)
{
	colortextmode_kprint(str, kstrlen(str), color);
}


