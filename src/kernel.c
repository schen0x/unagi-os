#include "kernel.h"
#include "config.h"
#include "util/kutil.h"
#include "idt/idt.h"
#include "io/io.h"
#include "memory/memory.h"
#include "memory/kheap.h"
#include "memory/paging/paging.h"
#include "disk/disk.h"
#include "fs/pathparser.h"
/* Kernel Page Directory */
static PAGE_DIRECTORY_4KB* kpd = 0;

void terminal_initialize()
{
	for (int64_t y = 0; y < VGA_HEIGHT; y++)
	{
		for (int64_t x = 0; x < VGA_WIDTH; x++ )
		{
			terminal_put_char(x, y, ' ', 0); // Fill screen with black
		}
	}
}

void kernel_main()
{
	terminal_initialize();
	char msg[] = "Hello World!Hello World!\nHello World!Hello World!\nHello World!Hello World!\nHello World!Hello World!\nHello World!Hello World!\nHello World!Hello World!\nHello World!Hello World!\nHello World!Hello World!\nHello World!Hello World!\nHello World!Hello World!\nHello World!Hello World!\nHello World!Hello World!\nHello World!Hello World!\nHello World!Hello World!\nHello World!Hello World!\nHello World!Hello World!\nHello World!Hello World!\nHello World!Hello World!\nHello World!Hello World!\nHello World!";
	size_t msg_len = kstrlen(msg);
	kprint(msg, msg_len, 4);
	char msg2[] = "    ";
	kprint(msg2, 5, 4);
	int32_t x = 10;
	int32_t y = 0;
	y = ++x;
	char n[10] = {0};
	kfprint(hex_to_ascii(n, &y, 4),4);

	// TODO CACHE OFF && MEMORY TEST
	// TODO e820 routine

	k_mm_init();
	idt_init();
	// ==============
	uint32_t pd_entries_flags = 0b111;
	kpd = pd_init(pd_entries_flags);
	paging_switch(kpd);
	enable_paging();

	disk_search_and_init();

	// ==============
	// TODO Try sprintf
	// TODO MOUSE HANDLING
	// TODO TERMINAL
	enable_interrupts();
	PATH_ROOT* root_path = path_parse("0:/bin/bash", NULL);
	(void) root_path;
}

static uint16_t* video_mem = (uint16_t*)(0xB8000); /* create a local pointer to the absolute address */
/* TODO multi thread handling */
static uint16_t current_terminal_row = 0; // y
static uint16_t current_terminal_col = 0; // x

uint16_t terminal_make_char(const char c, const uint8_t color)
{
	/* text mode, litten endian */
	return (color << 8) | c;
}

void terminal_put_char(const int64_t x, const int64_t y, const char c, const uint8_t color)
{
	/* Put char at x,y */

	video_mem[(y * VGA_WIDTH) + x] = terminal_make_char(c, color);
}

static void terminal_scroll()
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
		terminal_put_char(z, VGA_HEIGHT - 1, ' ', 0);
	}
}

static void current_terminal_row_scrolldown()
{
	current_terminal_row += 1;
	if (current_terminal_row >= VGA_HEIGHT)
	{
		terminal_scroll();
		current_terminal_row = VGA_HEIGHT - 1;
	}
}

void terminal_write_char(const char c, const uint8_t color)
{
	if (c == '\n')
	{
		current_terminal_col = 0;
		current_terminal_row_scrolldown();
		return;
	}

	terminal_put_char(current_terminal_col, current_terminal_row, c, color);
	current_terminal_col += 1;
	if (current_terminal_col >= VGA_WIDTH)
	{
		current_terminal_col = 0;
		current_terminal_row_scrolldown();
	}
}

void kprint(const char* str, const size_t len, const uint8_t color)
{
	char error_msg[] = "kernel.c/kprint: Input too long";
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

void kfprint(const char* str, const uint8_t color)
{
	kprint(str, kstrlen(str), color);
}


