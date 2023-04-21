#ifndef DRIVERS_GRAPHIC_VIDEOMODE_H_
#define DRIVERS_GRAPHIC_VIDEOMODE_H_

#include <stdint.h>
#include <stddef.h>
void videomode_window_initialize();
// void videomode_terminal_put_char(const int64_t x, const int64_t y, const char c, const uint8_t color);
// void videomode_kfprint(const char* str, const uint8_t color);
static void __draw_stripes();
static void __draw_three_boxes();
static void boxfill8(uint8_t* vram, int32_t xsize, uint8_t color, int32_t x0, int32_t y0, int32_t x1, int32_t y1);
static void set_palette(uint8_t start, uint8_t end, unsigned char *rgb);
static void init_palette(void);
static void draw_windows();

#endif
