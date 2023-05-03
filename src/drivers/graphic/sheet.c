#include "drivers/graphic/sheet.h"
#include "config.h"
#include "memory/memory.h"

SHTCTL* shtctl_init(uintptr_t vram, int32_t xsize, int32_t ysize)
{
	SHTCTL *ctl;
	ctl = (SHTCTL *)kzalloc(sizeof(SHTCTL));
	if (ctl == NULL)
		return NULL;
	ctl->vram = vram;
	ctl->xsize = xsize;
	ctl->ysize = ysize;
	ctl->zTop = -1; /* No sheet yet */
	for (int32_t i = 0; i < OS_VGA_MAX_SHEETS; i++)
	{
		ctl->sheet0[i].flags = 0; // unused
	}
	return ctl;
}


/**
 * Loop through ctl->sheet0, find a free sheet
 * Marks the sheet inUse, and z = -1
 * Return the pointer
 * Return NULL if all sheets are occupied
 */
SHEET* sheet_alloc(SHTCTL *ctl)
{
	SHEET *sheet;
	for (int32_t i = 0; i < OS_VGA_MAX_SHEETS; i++)
	{
		if (ctl->sheet0[i].flags == 0)
		{
			sheet = &ctl->sheet0[i];
			sheet->flags = SHEET_IN_USE; // in use
			sheet->z = -1; // is hidden
			return sheet;
		}
	}
	return NULL; // all sheets are occupied
}

void sheet_setbuf(SHEET *sheet, uint8_t *buf, int32_t xsize, int32_t ysize, int32_t color_invisible)
{
	sheet->buf = buf;
	sheet->bufXsize = xsize;
	sheet->bufYsize = ysize;
	sheet->color_invisible = color_invisible;
	return;
}

/**
 * Update the window, given an area.
 * TODO maybe in all situations xDst == xStart + ctl->sheets[z]->bufXsize
 *
 * @xStart: the real coordinate (screen) x of the sheet when start moving
 * @yStart: the real coordinate (screen) y of the sheet when start moving
 * @xEnd: the destination x, real coordinate (screen)
 * @yEnd: the destination y, real coordinate (screen)
 */
void sheet_update_with_screenxy(SHTCTL *ctl, int32_t xStartOnScreen, int32_t yStartOnScreen, int32_t xEndOnScreen, int32_t yEndOnScreen)
{
	uint8_t *buf, color;
	uint8_t *vram = (uint8_t *) ctl->vram;
	SHEET *sheet;
	/**
	 * Discard out of screen pixels
	 * Because otherwise it may cause wrap of undefined behavior
	 */
	if (xStartOnScreen < 0)
		xStartOnScreen = 0;
	if (xStartOnScreen > ctl->xsize)
		xStartOnScreen = ctl->xsize;
	if (xEndOnScreen < 0)
		xEndOnScreen = 0;
	if (xEndOnScreen > ctl->xsize)
		xEndOnScreen = ctl->xsize;
	if (yStartOnScreen < 0)
		yStartOnScreen = 0;
	if (yStartOnScreen > ctl->ysize)
		yStartOnScreen = ctl->ysize;
	if (yEndOnScreen < 0)
		yEndOnScreen = 0;
	if (yEndOnScreen > ctl->ysize)
		yEndOnScreen = ctl->ysize;


	/* Loop and draw all visible SHEETs from bottom to top */
	for (int32_t z = 0; z <= ctl->zTop; z++)
	{
		sheet = ctl->sheets[z];
		buf = sheet->buf;

		/* Calculate the relative xy in a buffer, from the screen xy */
		int32_t xStartInBuf = xStartOnScreen - sheet->xStart;
		int32_t yStartInBuf = yStartOnScreen - sheet->yStart;
		int32_t xEndInBuf = xEndOnScreen - sheet->xStart;
		int32_t yEndInBuf = yEndOnScreen - sheet->yStart;

		/* Only loop through the affected part */
		if (xStartInBuf < 0)
			xStartInBuf = 0;
		if (yStartInBuf < 0)
			yStartInBuf = 0;
		if (xEndInBuf > sheet->bufXsize)
			xEndInBuf = sheet->bufXsize;
		if (yEndInBuf > sheet->bufYsize)
			yEndInBuf = sheet->bufYsize;

		for (int32_t bufY = yStartInBuf; bufY < yEndInBuf; bufY++)
		{
			int32_t y = sheet->yStart + bufY;
			for (int32_t bufX = xStartInBuf; bufX < xEndInBuf; bufX++)
			{
				int32_t x = sheet->xStart + bufX;
				color = buf[bufY * sheet->bufXsize + bufX];
				if (color != sheet->color_invisible)
				{
					vram[y * ctl->xsize + x] = color;
				}
			}
		}
	}
	return;
}
/**
 * Update a sheet
 */
void sheet_update_with_bufxy(SHTCTL *ctl, SHEET *s, int32_t xStartInBuf, int32_t yStartInBuf, int32_t xEndInBuf, int32_t yEndInBuf)
{
	if (s->z < 0)
		return;
	sheet_update_with_screenxy(ctl, s->xStart + xStartInBuf, s->yStart + yStartInBuf, s->xStart + xEndInBuf, s->yStart + yEndInBuf);
	return;
}

/**
 * Update all sheets
 * TODO
 */
void sheet_update_all(SHTCTL *ctl)
{
	uint8_t *buf, color;
	uint8_t *vram = (uint8_t *) ctl->vram;
	SHEET *sheet;

	/* Loop and draw all visible SHEETs from bottom to top */
	for (int32_t z = 0; z <= ctl->zTop; z++)
	{
		sheet = ctl->sheets[z];
		buf = sheet->buf;
		for (int32_t bufY = 0; bufY < sheet->bufYsize; bufY++)
		{
			int32_t y = sheet->yStart + bufY;
			for (int32_t bufX = 0; bufX < sheet->bufXsize; bufX++)
			{
				int32_t x = sheet->xStart + bufX;
				color = buf[bufY * sheet->bufXsize + bufX];
				if (color != sheet->color_invisible)
				{
					vram[y * ctl->xsize + x] = color;
				}
			}
		}
	}

}

/*
 * Set zNew to a new sheet
 * @sheet The sheet to adjust zIndex
 * @zNew New zIndex
 */
void sheet_updown(SHTCTL *ctl, SHEET *sheet, int32_t zNew)
{
	int32_t z, zOriginal = sheet->z;

	/* Normalize the zNew */
	if (zNew > ctl->zTop + 1)
	{
		zNew = ctl->zTop + 1;
	}
	if (zNew < -1)
	{
		zNew = -1;
	}

	sheet->z = zNew;

	if (zOriginal == zNew)
		return;

	/* To hide a sheet  */
	if (zNew < 0)
	{
		/*
		 * Discard the original ctl->sheet[zIndex]
		 * Move everything atop DOWN by 1
		 */
		for (z = zOriginal; z < ctl->zTop; z++)
		{
			ctl->sheets[z] = ctl->sheets[z + 1];
			ctl->sheets[z]->z = z;
		}
		/* As a result, total height is 1 less */
		ctl->zTop--;
		sheet_update_with_screenxy(ctl, sheet->xStart, sheet->yStart, sheet->xStart + sheet->bufXsize, sheet->yStart + sheet->bufYsize);
		return;
	}

	/* To display what was hidden */
	if (zOriginal < 0)
	{
		 /* Move everything atop UP by 1 to make space */
		for (z = ctl->zTop; z > zNew; z--)
		{
			ctl->sheets[z] = ctl->sheets[z - 1];
			ctl->sheets[z]->z = z;
		}
		/* Destined position is now empty, write it */
		ctl->sheets[zNew] = sheet;
		/* As a result, total height is 1 higher */
		ctl->zTop++;
		sheet_update_with_screenxy(ctl, sheet->xStart, sheet->yStart, sheet->xStart + sheet->bufXsize, sheet->yStart + sheet->bufYsize);
		return;
	}

	/* Move something downwards (normal) (zOriginal > zNew >= 0) */
	if (zOriginal > zNew)
	{
		/* Move everything below UP by 1 to make space */
		for (z = zOriginal; z > zNew; z--)
		{
			ctl->sheets[z] = ctl->sheets[z - 1];
			ctl->sheets[z]->z = z;
		}
		/* Destined position is now empty, write it */
		ctl->sheets[zNew] = sheet;
		sheet_update_with_screenxy(ctl, sheet->xStart, sheet->yStart, sheet->xStart + sheet->bufXsize, sheet->yStart + sheet->bufYsize);
		return;
	}

	/*
	 * Moving something up (normal), (zNew > zOriginal >= 0)
	 */
	for (z = zOriginal; z < zNew; z++)
	{
	 	/* Move everything atop DOWN by 1 to make space */
		ctl->sheets[z] = ctl->sheets[z + 1];
		ctl->sheets[z]->z = z;
	}
	/* Destined position is now empty, write it */
	ctl->sheets[zNew] = sheet;
	sheet_update_with_screenxy(ctl, sheet->xStart, sheet->yStart, sheet->xStart + sheet->bufXsize, sheet->yStart + sheet->bufYsize);
	return;
}

/**
 * Move the sheet to a new coordinate on the screen
 * @xDst Destination x
 * @yDst Destination y
 */
void sheet_slide(SHTCTL *ctl, SHEET *sheet, int32_t xDst, int32_t yDst)
{
	int32_t xStart = sheet->xStart;
	int32_t yStart = sheet->yStart;
	sheet->xStart = xDst;
	sheet->yStart = yDst;

	/* If sheet is hidden, the position should still be updated, but do not render */
	if (sheet->z < 0)
		return;
	// sheet_refresh(ctl);
	sheet_update_with_screenxy(ctl, xStart, yStart, xStart + sheet->bufXsize, yStart + sheet->bufYsize);
	sheet_update_with_screenxy(ctl, xDst, yDst, xDst + sheet->bufXsize, yDst + sheet->bufYsize);
	return;
}

void sheet_free(SHTCTL *ctl, SHEET *sheet)
{
	if (sheet->z >= 0)
	{
		sheet_updown(ctl, sheet, -1);
	}
	sheet->flags = 0;
	return;
}

