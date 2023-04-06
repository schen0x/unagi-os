#include "kernel.h"
#include "config.h"
#include "util/kutil.h"
#include "idt/idt.h"
#include "io/io.h"
extern void problem();

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
	char msg[] = "Hello World!\nHello World!";
	// int64_t msg_len = kstrlen(msg);
	// size_t msg_len = (sizeof(msg) / sizeof(msg[0]) - 1); /* sizeof (array) includes the null byte */
	size_t msg_len = kstrlen(msg);
	kprint(msg, msg_len, 4);

	uint32_t h = 0x123456f8;
	//! void* hex_number = (void*)0x123456f8; /* pointer points to the absolute address 0x123456f8 */
	char ascii_str[2 * sizeof(h) + 1]; // +1 for null terminator
	hex_to_ascii(&h, ascii_str, sizeof(h));
	kprint(ascii_str, 2*sizeof(h), 4);
	idt_init();

	outb(0x60, 0xff);

	problem();
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

void terminal_write_char(const char c, const uint8_t color)
{
	if (c == '\n')
	{
		current_terminal_row += 1;
		current_terminal_col = 0;
		return;
	}

	terminal_put_char(current_terminal_col, current_terminal_row, c, color);
	current_terminal_col += 1;
	if (current_terminal_col >= VGA_WIDTH)
	{
		current_terminal_col = 0;
		current_terminal_row += 1;
	}
}

void kprint(const char* str, const size_t len, const uint8_t color)
{
	char error_msg[] = "kernel.c/kprint: Panic! Wrong Input";
	if (len > 512)
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

