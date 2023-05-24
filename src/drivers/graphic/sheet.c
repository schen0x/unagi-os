#include "drivers/graphic/sheet.h"
#include "config.h"
#include "memory/memory.h"

SHTCTL* shtctl_init(uintptr_t vram, int32_t xsize, int32_t ysize)
{
	SHTCTL *ctl;
	ctl = (SHTCTL *)kzalloc(sizeof(SHTCTL));
	if (ctl == NULL)
		return NULL;
	ctl->zMap = (uint8_t *) kzalloc(xsize * ysize);
	if (ctl->zMap == NULL)
	{
		kfree(ctl);
		return NULL;
	}
	ctl->vram = vram;
	ctl->xsize = xsize;
	ctl->ysize = ysize;
	ctl->zTop = -1; /* No sheet yet */
	for (int32_t i = 0; i < OS_VGA_MAX_SHEETS; i++)
	{
		ctl->sheet0[i].flags = 0; // unused
		ctl->sheet0[i].ctl = ctl;
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
 * Update the zMap given a region on the screen
 * @xStart: the real coordinate (screen) x of the sheet when start moving
 * @yStart: the real coordinate (screen) y of the sheet when start moving
 * @xEnd: the destination x, real coordinate (screen)
 * @yEnd: the destination y, real coordinate (screen)
 * @zStart: sheet->z; (to update only the sheets with z >= zStart)
 */
void sheet_update_zmap(SHTCTL *ctl, int32_t xStartOnScreen, int32_t yStartOnScreen, int32_t xEndOnScreen, int32_t yEndOnScreen, int32_t zStart)
{
	uint8_t *buf, color;
	uint8_t *zMap = (uint8_t *) ctl->zMap;
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


	/**
	 * Loop and draw all visible SHEETs from bottom to top,
	 * starting from the zStart
	 * (the highest SHEET known visible that is changed)
	 */
	for (int32_t z = zStart; z <= ctl->zTop; z++)
	{
		sheet = ctl->sheets[z];
		/**
		 * Calculate the index of the "buffer" in `sheet0`
		 * sheet == sheet0[sheetId] is the real memory location of the graphic
		 * But sheeId does not change when moving up or down
		 * Write it to the `zMap`
		 */
		int32_t sheetId = sheet - ctl->sheet0; // pointer arithmetic, gives index
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
					zMap[ctl->xsize * y + x] = sheetId;
				}
			}
		}
	}
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
 * @zStart: sheet->z; (to update only the sheets with z >= zStart)
 * @zEnd: sheet->z; (to update only the sheets with z <= zEnd), if zEnd == -1, set zEnd to `ctl->zTop`
 */
void sheet_update_with_screenxy(SHTCTL *ctl, int32_t xStartOnScreen, int32_t yStartOnScreen, int32_t xEndOnScreen, int32_t yEndOnScreen, int32_t zStart, int32_t zEnd)
{
	uint8_t *buf, color;
	uint8_t *vram = (uint8_t *) ctl->vram;
	uint8_t *zMap = (uint8_t *) ctl->zMap;
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

	if (zEnd == -1)
		zEnd = ctl->zTop;

	/**
	 * Loop and draw all visible SHEETs from bottom to top,
	 * starting from the zStart
	 * (the highest SHEET known visible that is changed)
	 */
	for (int32_t z = zStart; z <= zEnd; z++)
	{
		sheet = ctl->sheets[z];
		buf = sheet->buf;
		int32_t sheetId = sheet - ctl->sheet0;

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
				color = buf[sheet->bufXsize * bufY + bufX];
				if (zMap[ctl->xsize * y + x] == sheetId)
				{
					vram[ctl->xsize * y + x] = color;
				}
			}
		}
	}
	return;
}
/**
 * Update a sheet and redraw all sheets above
 * @s SHEET*
 * @xStartInBuf relative coordinate in the buffer
 * @yStartInBuf relative coordinate in the buffer
 * @xEndInBuf relative coordinate in the buffer
 * @yEndInBuf relative coordinate in the buffer
 * The EndInBuf can be overbound (will be normalize)
 *
 */
void sheet_update_sheet(SHEET *s, int32_t xStartInBuf, int32_t yStartInBuf, int32_t xEndInBuf, int32_t yEndInBuf)
{
	if (s->z < 0)
		return;
	/* Assume the zMap is correct, only one (this) sheet needs redraw */
	sheet_update_with_screenxy(s->ctl, s->xStart + xStartInBuf, s->yStart + yStartInBuf, s->xStart + xEndInBuf, s->yStart + yEndInBuf, s->z, s->z);
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
	uint8_t *zMap = (uint8_t *) ctl->zMap;

	/* Loop and draw all visible SHEETs from bottom to top */
	for (int32_t z = 0; z <= ctl->zTop; z++)
	{
		sheet = ctl->sheets[z];
		buf = sheet->buf;
		int32_t sheetId = sheet - ctl->sheet0;
		for (int32_t bufY = 0; bufY < sheet->bufYsize; bufY++)
		{
			int32_t y = sheet->yStart + bufY;
			for (int32_t bufX = 0; bufX < sheet->bufXsize; bufX++)
			{
				int32_t x = sheet->xStart + bufX;
				color = buf[sheet->bufXsize * bufY + bufX];
				if (zMap[ctl->xsize * y + x] == sheetId)
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
void sheet_updown(SHEET *sheet, int32_t zNew)
{
	SHTCTL *ctl = sheet->ctl;
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
		sheet_update_zmap(ctl, sheet->xStart, sheet->yStart, sheet->xStart + sheet->bufXsize, sheet->yStart + sheet->bufYsize, 0);
		sheet_update_with_screenxy(ctl, sheet->xStart, sheet->yStart, sheet->xStart + sheet->bufXsize, sheet->yStart + sheet->bufYsize, 0, zOriginal - 1);
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
		sheet_update_zmap(ctl, sheet->xStart, sheet->yStart, sheet->xStart + sheet->bufXsize, sheet->yStart + sheet->bufYsize, zNew);
		sheet_update_with_screenxy(ctl, sheet->xStart, sheet->yStart, sheet->xStart + sheet->bufXsize, sheet->yStart + sheet->bufYsize, zNew, zNew);
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
		sheet_update_zmap(ctl, sheet->xStart, sheet->yStart, sheet->xStart + sheet->bufXsize, sheet->yStart + sheet->bufYsize, zNew);
		sheet_update_with_screenxy(ctl, sheet->xStart, sheet->yStart, sheet->xStart + sheet->bufXsize, sheet->yStart + sheet->bufYsize, zNew, zOriginal - 1);
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
	sheet_update_zmap(ctl, sheet->xStart, sheet->yStart, sheet->xStart + sheet->bufXsize, sheet->yStart + sheet->bufYsize, zNew);
	sheet_update_with_screenxy(ctl, sheet->xStart, sheet->yStart, sheet->xStart + sheet->bufXsize, sheet->yStart + sheet->bufYsize, zNew, zNew);
	return;
}

/**
 * Move the sheet to a new coordinate on the screen
 * @xDst Destination x
 * @yDst Destination y
 */
void sheet_slide(SHEET *sheet, int32_t xDst, int32_t yDst)
{
	SHTCTL *ctl = sheet->ctl;
	int32_t xStart = sheet->xStart;
	int32_t yStart = sheet->yStart;
	sheet->xStart = xDst;
	sheet->yStart = yDst;

	/* If sheet is hidden, the position should still be updated, but do not render */
	if (sheet->z < 0)
		return;
	sheet_update_zmap(ctl, xStart, yStart, xStart + sheet->bufXsize, yStart + sheet->bufYsize, 0);
	sheet_update_zmap(ctl, xDst, yDst, xDst + sheet->bufXsize, yDst + sheet->bufYsize, sheet->z);
	/* Redraw the background (z < this.z) when leaving */
	sheet_update_with_screenxy(ctl, xStart, yStart, xStart + sheet->bufXsize, yStart + sheet->bufYsize, 0, sheet->z - 1);
	/* Redraw the dst */
	sheet_update_with_screenxy(ctl, xDst, yDst, xDst + sheet->bufXsize, yDst + sheet->bufYsize, sheet->z, sheet->z);
	return;
}

void sheet_free(SHEET *sheet)
{
	if (sheet->z >= 0)
	{
		sheet_updown(sheet, -1);
	}
	sheet->flags = 0;
	return;
}

