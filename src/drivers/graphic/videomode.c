#include "drivers/graphic/videomode.h"
#include <stdint.h>
#include <stddef.h>
#include "io/io.h"

// static uint16_t* video_mem = (uint16_t*)(0xa8000); /* create a local pointer to the absolute address */

// void videomode_terminal_put_char(const int64_t x, const int64_t y, const char c, const uint8_t color);
// void videomode_kfprint(const char* str, const uint8_t color);
void videomode_terminal_initialize()
{
	unsigned char* p = NULL;
	// use intptr_t to ensure the size is big enough to hold a pointer
	for (intptr_t i = 0xa0000; i <= 0xaffff; i++)
	{
		// _asm_write_mem8(i, 15); // all white
		// _asm_write_mem8(i, i & 0x0f); // vertical stripes
		p = (unsigned char *)i;
		*p = i & 0x0f;
		*(uint8_t*) i = i & 0x0f; // same
	}

}

// TODO
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

