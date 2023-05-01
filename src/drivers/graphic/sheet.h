#ifndef DRIVERS_GRAPHIC_SHEET_H_
#define DRIVERS_GRAPHIC_SHEET_H_

#include "config.h"
#include <stdint.h>

typedef struct SHEET
{
	uint8_t *buf;
	int32_t bxsize, bysize, vx0, vy0, col_inv, height, flags;
} SHEET;

typedef struct SHTCTL
{
	uintptr_t vram;
	int32_t xsize, ysize, top;
	SHEET *sheets[OS_VGA_MAX_SHEETS];
	SHEET sheet0[OS_VGA_MAX_SHEETS];
} SHTCTL;

#endif
