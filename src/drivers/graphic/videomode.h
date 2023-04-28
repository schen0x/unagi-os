#ifndef DRIVERS_GRAPHIC_VIDEOMODE_H_
#define DRIVERS_GRAPHIC_VIDEOMODE_H_

#include <stdint.h>
#include <stddef.h>
#include "include/uapi/bootinfo.h"
void videomode_window_initialize(BOOTINFO* bi);
// void videomode_terminal_put_char(const int64_t x, const int64_t y, const char c, const uint8_t color);
// void videomode_kfprint(const char* str, const uint8_t color);
static void __draw_stripes();
static void __draw_three_boxes();
static void __write_some_chars(BOOTINFO* bi);
static void boxfill8(uintptr_t vram, int32_t xsize, uint8_t color, int32_t x0, int32_t y0, int32_t x1, int32_t y1);
static void set_palette(uint8_t start, uint8_t end, unsigned char *rgb);
static void init_palette(void);
static void draw_windows(uintptr_t vram, int32_t screenXsize, int32_t screenYsize);
void init_mouse_cursor8(intptr_t vram_mouse_address, uint8_t back_color);
void putblock8_8(intptr_t vram, int32_t vxsize, int32_t pxsize, int32_t pysize, int32_t px0, int32_t py0, uint8_t* buf, int32_t bxsize);
void videomode_kfprint(const char* str, const uint8_t color);

#endif
