#include "kernel.h"
#include "config.h"
#include "util/kutil.h"
#include "idt/idt.h"
#include "io/io.h"
#include "memory/memory.h"
#include "memory/kheap.h"
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
	size_t msg_len = kstrlen(msg);
	kprint(msg, msg_len, 4);

	uint32_t h = 0x123456f8;
	//! void* hex_number = (void*)0x123456f8; /* pointer points to the absolute address 0x123456f8 */
	char ascii_str_buf[2 * sizeof(h) + 1]; // +1 for null terminator
	hex_to_ascii(ascii_str_buf, &h, sizeof(h));
	kprint(ascii_str_buf, 2*sizeof(h), 4);

	// TODO CACHE OFF && MEMORY TEST
	// uint8_t* memory_start = (uint8_t*) OS_HEAP_ADDRESS;
	// kmemory_init(memory_start, OS_HEAP_SIZE_BYTES - 1); // ? -1 FIXME FIX & CONFIRM LATER

	k_heap_table_mm_init(); // managemend by simple heap table

	idt_init();
	enable_interrupts();

	//char* ptr = (char*)kmalloc(2*sizeof(char));
	char* ptr = (char*)k_heap_table_mm_malloc(2*sizeof(char));
	char* ptr2 = (char*)k_heap_table_mm_malloc(5000);
	ptr2[0] ='B';
	//(gdb) p ptr2
	// $3 = 0x2002000 "B"
	// (gdb) p ptr
	// $4 = 0x2000000 ""
	//
	for (int i = 0; i<500;i+=8)
	{
		for (int j = 0; j<8;j++)
		{
			ptr[i+j] = 0x41+j;
		}
	}
	// ptr[10] = 0;
	kfprint(ptr, 4);
	// char ascii_str2[2 + 1 + 4];
	//hex_to_ascii(&ptr, ascii_str2, (2+1+4));
	//kprint(ascii_str2);
	kfree((void*)ptr);


	// TODO IMPLEMENT PRINTF!
	//
	// TODO MOUSE HANDLING
	// TODO TERMINAL
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

void kfprint(const char* str, const uint8_t color)
{
	kprint(str, kstrlen(str), color);
}


