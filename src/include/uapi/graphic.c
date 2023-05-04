#include "include/uapi/graphic.h"
#include "drivers/graphic/colortextmode.h"
#include "drivers/graphic/videomode.h"
#include "include/uapi/bootinfo.h"

void graphic_init(BOOTINFO* bi)
{
	// colortextmode_terminal_initialize();
	videomode_window_initialize(bi);
}

void putchar(const int64_t x, const int64_t y, const char c, const uint8_t color)
{
	colortextmode_terminal_put_char(x,y,c,color);

}
void kfprint(const char* str, const uint8_t color)
{
	// colortextmode_kfprint(str,color);
	videomode_kfprint(str, color);
}

SHTCTL* graphic_window_manager_init(BOOTINFO* bi)
{
	return sheet_initialize((uintptr_t)bi->vram, bi->scrnx, bi->scrny);
}
