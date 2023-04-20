#include "drivers/graphic/videomode.h"
#include <stdint.h>
#include <stddef.h>

// static uint16_t* video_mem = (uint16_t*)(0xa8000); /* create a local pointer to the absolute address */

// void videomode_terminal_put_char(const int64_t x, const int64_t y, const char c, const uint8_t color);
// void videomode_kfprint(const char* str, const uint8_t color);
extern void _write_mem8(uint32_t addr, uint8_t data);
void videomode_terminal_initialize()
{
	for (int32_t i = 0xa0000; i <= 0xaffff; i++)
	{
		_write_mem8(i, 15); // all white
	}

}
