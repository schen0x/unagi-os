#ifndef DRIVERS_GRAPHIC_VIDEOMODE_H_
#define DRIVERS_GRAPHIC_VIDEOMODE_H_

#include <stdint.h>
#include <stddef.h>
#include "include/uapi/bootinfo.h"
#include "include/uapi/input-mouse-event.h"
#include "idt/idt.h"
#include "drivers/graphic/sheet.h"

#define COL8_000000 0 // 0:Black
#define COL8_FF0000 1 // 1:Red+
#define COL8_00FF00 2 // 2:Green+
#define COL8_FFFF00 3 // 3:Yellow+
#define COL8_0000FF 4 // 4:Cyan+
#define COL8_FF00FF 5 // 5:Purple+
#define COL8_00FFFF 6 // 6:Blue+
#define COL8_FFFFFF 7 // 7:White
#define COL8_C6C6C6 8 // 8:Gray+
#define COL8_840000 9 // 9:Red-
#define COL8_008400 10 // 10:Green-
#define COL8_848400 11 // 11:Yellow-
#define COL8_000084 12 // 12:Cyan-
#define COL8_840084 13 // 13:Purple-
#define COL8_008484 14 // 14:Blue-
#define COL8_848484 15 // 15:Gray-

void videomode_window_initialize(BOOTINFO* bi);
static void __draw_stripes();
static void __draw_three_boxes();
static void __write_some_chars(BOOTINFO* bi);
static void boxfill8(uintptr_t vram, int32_t xsize, uint8_t color, int32_t x0, int32_t y0, int32_t x1, int32_t y1);
static void set_palette(uint8_t start, uint8_t end, unsigned char *rgb);
static void init_block_fill(uint8_t *block_start, const uint8_t filling_color, const size_t block_size_in_bytes);
static void init_palette(void);
static void draw_windows(uintptr_t vram, int32_t screenXsize, int32_t screenYsize);
void init_mouse_cursor8(intptr_t vram_mouse_address, uint8_t back_color);
void putblock8_8(intptr_t vram, int32_t vxsize, int32_t pxsize, int32_t pysize, int32_t px0, int32_t py0, uint8_t* buf, int32_t bxsize);
void videomode_kfprint(const char* str, uint8_t color);
void graphic_move_mouse(MOUSE_DATA_BUNDLE *mouse_one_move);
static void display_scroll(uintptr_t vram, int32_t vga_width, int32_t vga_height);
SHTCTL* sheet_initialize(uintptr_t vram, int32_t scrnx, int32_t scrny);
void putfonts8_asc(uintptr_t vram, int32_t xsize, int32_t x, int32_t y, uint8_t color, char *s);
SHTCTL* sheet_initialize(uintptr_t vram, int32_t scrnx, int32_t scrny);

#endif
