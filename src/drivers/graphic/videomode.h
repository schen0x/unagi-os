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

/**
 * Returned structure of int10h ax=0x4F00;
 * FUNCTION: Get VESA BIOS information
 * Description: Returns the VESA BIOS information, including manufacturer,
 * supported modes, available video memory, etc...
 *
 * Input: AX = 0x4F00
 * Input: ES:DI = Segment:Offset pointer to where to store VESA BIOS information structure.
 * Output: AX = 0x004F on success, other values indicate that VESA BIOS is not supported.
 */
typedef struct VBE_INFO {
	char signature[4];		// must be "VESA" to indicate valid VBE support
	uint16_t version;		// VBE version; high byte is major version, low byte is minor version
	uint32_t oem;			// segment:offset pointer to OEM
	uint32_t capabilities;		// bitfield that describes card capabilities
	uint32_t video_modes;		// segment:offset pointer to list of supported video modes
	uint16_t video_memory;		// amount of video memory in 64KB blocks
	uint16_t software_rev;		// software revision
	uint32_t vendor;			// segment:offset to card vendor string
	uint32_t product_name;		// segment:offset to card model name
	uint32_t product_rev;		// segment:offset pointer to product revision
	char reserved[222];		// reserved for future expansion
	char oem_data[256];		// OEM BIOSes store their strings in this area
} __attribute__((packed)) VBE_INFO;

/**
 * Returned structure of int10h ax=0x4F01;
 * FUNCTION: Get VESA mode information
 * Description: This function returns the mode information structure for a
 * specified mode. The mode number should be gotten from the supported modes
 * array.
 *
 * Input: AX = 0x4F01
 * Input: CX = VESA mode number from the video modes array
 * Input: ES:DI = Segment:Offset pointer of where to store the VESA Mode Information Structure shown below.
 * Output: AX = 0x004F on success, other values indicate a BIOS error or a mode-not-supported error.
 */
typedef struct VBE_MODE_INFO {
	uint16_t attributes;		// deprecated, only bit 7 should be of interest to you, and it indicates the mode supports a linear frame buffer.
	uint8_t window_a;			// deprecated
	uint8_t window_b;			// deprecated
	uint16_t granularity;		// deprecated; used while calculating bank numbers
	uint16_t window_size;
	uint16_t segment_a;
	uint16_t segment_b;
	uint32_t win_func_ptr;		// deprecated; used to switch banks from protected mode without returning to real mode
	uint16_t pitch;			// number of bytes per horizontal line
	uint16_t width;			// width in pixels
	uint16_t height;			// height in pixels
	uint8_t w_char;			// unused...
	uint8_t y_char;			// ...
	uint8_t planes;
	uint8_t bpp;			// bits per pixel in this mode
	uint8_t banks;			// deprecated; total number of banks in this mode
	uint8_t memory_model;
	uint8_t bank_size;		// deprecated; size of a bank, almost always 64 KB but may be 16 KB...
	uint8_t image_pages;
	uint8_t reserved0;

	uint8_t red_mask;
	uint8_t red_position;
	uint8_t green_mask;
	uint8_t green_position;
	uint8_t blue_mask;
	uint8_t blue_position;
	uint8_t reserved_mask;
	uint8_t reserved_position;
	uint8_t direct_color_attributes;

	uint32_t framebuffer;		// physical address of the linear frame buffer; write here to draw to the screen
	uint32_t off_screen_mem_off;
	uint16_t off_screen_mem_size;	// size of memory in the framebuffer but not being displayed on the screen
	uint8_t reserved1[206];
} __attribute__ ((packed)) VBE_MODE_INFO;

typedef struct SCREEN_MOUSEXY {
	int32_t mouseX;
	int32_t mouseY;
} SCREEN_MOUSEXY;

void videomode_window_initialize(BOOTINFO* bi);
static void __draw_stripes();
static void __draw_three_boxes();
static void __write_some_chars(BOOTINFO* bi);
void boxfill8(uintptr_t vram, int32_t xsize, uint8_t color, int32_t x0, int32_t y0, int32_t x1, int32_t y1);
static void set_palette(uint8_t start, uint8_t end, unsigned char *rgb);
static void init_block_fill(uint8_t *block_start, const uint8_t filling_color, const size_t block_size_in_bytes);
static void init_palette(void);
static void draw_desktop(uintptr_t vram, int32_t screenXsize, int32_t screenYsize);
void init_mouse_cursor8(intptr_t mouseBuf, uint8_t back_color);
void putblock8_8(intptr_t vram, int32_t vxsize, int32_t pxsize, int32_t pysize, int32_t px0, int32_t py0, uint8_t* buf, int32_t bxsize);
void videomode_kfprint(const char* str, uint8_t color);
void graphic_move_mouse(MOUSE_DATA_BUNDLE *mouse_one_move);
static void display_scroll(uintptr_t vram, int32_t vga_width, int32_t vga_height);
SHTCTL* sheet_initialize(uintptr_t vram, int32_t scrnx, int32_t scrny);
void putfonts8_asc(uintptr_t vram, int32_t xsize, int32_t x, int32_t y, uint8_t color, char *s);
SHEET* get_sheet_window();
SHEET* get_sheet_console();
SCREEN_MOUSEXY* getMouseXY(SCREEN_MOUSEXY *xy);
bool isCursorWithinSheet(const SCREEN_MOUSEXY *xy, const SHEET *s);
void make_window8(uintptr_t buf, int xsize, int ysize, char *title, bool isFocus);
void make_textbox8(SHEET *sheet, int32_t xStart, int32_t yStart, int32_t width, int32_t height, int8_t color);
void make_wtitle8(uintptr_t buf, int xsize, int ysize, char *title, bool isFocus);
SHEET* get_sheet_by_cursor(const SCREEN_MOUSEXY *xy);
#endif
