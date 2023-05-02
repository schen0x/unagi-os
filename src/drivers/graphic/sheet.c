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


SHEET* sheet_alloc(SHTCTL *ctl)
{
	SHEET *sht;
	for (int32_t i = 0; i < OS_VGA_MAX_SHEETS; i++)
	{
		if (ctl->sheet0[i].flags == 0)
		{
			sht = &ctl->sheet0[i];
			sht->flags = SHEET_IN_USE; // in use
			sht->z = -1; // is hidden
			return sht;
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
 * Update the window
 */
void sheet_refresh(SHTCTL *ctl)
{
	uint8_t *buf, color;
	uint8_t *vram = (uint8_t *) ctl->vram;
	SHEET *sheet;

	/* Loop throgth all visible SHEETs */
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
		sheet_refresh(ctl);
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
		sheet_refresh(ctl);
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
		sheet_refresh(ctl);
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
	sheet_refresh(ctl);
	return;
}

/* Set the starting coordinate on the screen of the sheet */
void sheet_slide(SHTCTL *ctl, SHEET *sheet, int32_t xStart, int32_t yStart)
{
	sheet->xStart = xStart;
	sheet->yStart = yStart;
	if (sheet->z >= 0)
	{
		sheet_refresh(ctl);
	}
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

