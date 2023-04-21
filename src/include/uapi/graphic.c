#include "include/uapi/graphic.h"
#include "drivers/graphic/colortextmode.h"
#include "drivers/graphic/videomode.h"

void graphic_initialize()
{
	// colortextmode_terminal_initialize();
	videomode_window_initialize();
}

void putchar(const int64_t x, const int64_t y, const char c, const uint8_t color)
{
	colortextmode_terminal_put_char(x,y,c,color);

}
void kfprint(const char* str, const uint8_t color)
{
	colortextmode_kfprint(str,color);
}

