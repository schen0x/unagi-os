#include "kernel.h"

static uint16_t* video_mem = (uint16_t*)(0xB8000); /* create a local pointer to the absolute address */
static uint16_t current_terminal_row = 0; // y
static uint16_t current_terminal_col = 0; // x

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
}

void kstrcpy(char* dest, const char* src)
{
	while (*src)
	{
		*dest++ = *src++;
	}
	*dest = '\0';
}

size_t kstrlen(const char *str)
{
	size_t len = 0;
	while (str[len] != '\0')
	{
		len++;
	}
	return len;
}

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
	for (size_t i = 0; i < len; i++)
	{
		terminal_write_char(str[i], color);
	}
}