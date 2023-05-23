#ifndef DRIVERS_GRAPHIC_SHEET_H_
#define DRIVERS_GRAPHIC_SHEET_H_

#include "config.h"
#include <stdint.h>
#define SHEET_IN_USE 1

typedef struct SHEET
{
	uint8_t *buf;
	/* @bxsize: WIDTH of the SHEET */
	int32_t bufXsize;
	/* @bysize: HEIGHT of the SHEET */
	int32_t bufYsize;
	/* @vx0: starting coordinate x */
	int32_t xStart;
	/* @vy0: starting coordinate y */
	int32_t yStart;
	/* @col_inv: color */
	int32_t color_invisible;
	/* @height: z index; -1 hidden, [-1, SHTCTL->zTop]  */
	int32_t z;
	/* @flags: SHEET_IN_USE == 1 */
	int32_t flags;
	struct SHTCTL *ctl;
} SHEET;

typedef struct SHTCTL
{
	uintptr_t vram;
	/* @map: map<pos, z>; Find the top-most sheet that should be rendered, given a position */
	uint8_t* zMap;
	int32_t xsize, ysize;
	/* @top: max z of the top sheet */
	int32_t zTop;
	/*
	 * @sheets: an array of SHEET pointer, arranged based on zIndex
	 * i.e., sheet with z==0 is sheets[0], for z==3 is sheets[3]
	 */
	SHEET *sheets[OS_VGA_MAX_SHEETS];
	/* @sheet0: an array of SHEET structure; size: sizeof(SHEET) * OS_VGA_MAX_SHEETS */
	SHEET sheet0[OS_VGA_MAX_SHEETS];
} SHTCTL;
SHTCTL *shtctl_init(uintptr_t vram, int32_t xsize, int32_t ysize);
SHEET *sheet_alloc(SHTCTL *ctl);
void sheet_setbuf(SHEET *sheet, uint8_t *buf, int32_t xsize, int32_t ysize, int32_t color_invisible);
void sheet_update_all(SHTCTL *ctl);
void sheet_update_zmap(SHTCTL *ctl, int32_t xStartOnScreen, int32_t yStartOnScreen, int32_t xEndOnScreen, int32_t yEndOnScreen, int32_t zStart);
void sheet_update_with_screenxy(SHTCTL *ctl, int32_t xStartOnScreen, int32_t yStartOnScreen, int32_t xEndOnScreen, int32_t yEndOnScreen, int32_t zStart, int32_t zEnd);

void sheet_update_sheet(SHEET *s, int32_t xStartInBuf, int32_t yStartInBuf, int32_t xEndInBuf, int32_t yEndInBuf);
void sheet_updown(SHEET *sheet, int32_t zNew);
void sheet_slide(SHEET *sheet, int32_t xDst, int32_t yDst);
void sheet_free(SHEET *sheet);

#endif
