#include "drivers/graphic/sheet.h"
#include "config.h"
#include "memory/memory.h"
#define SHEET_IN_USE 1

SHTCTL *shtctl_init(uintptr_t vram, int32_t xsize, int32_t ysize)
{
	SHTCTL *ctl;
	ctl = (SHTCTL *)kzalloc(sizeof(SHTCTL));
	if (ctl == NULL)
		return NULL;
	ctl->vram = vram;
	ctl->xsize = xsize;
	ctl->ysize = ysize;
	ctl->top = -1; /* No sheet yet */
	for (int32_t i = 0; i < OS_VGA_MAX_SHEETS; i++)
	{
		ctl->sheet0[i].flags = 0; // unused
	}
	return ctl;
}


SHEET *SHEET_ALLOC(SHTCTL *ctl)
{
	SHEET *sht;
	for (int32_t i = 0; i < OS_VGA_MAX_SHEETS; i++)
	{
		if (ctl->sheet0[i].flags == 0)
		{
			sht = &ctl->sheet0[i];
			sht->flags = SHEET_IN_USE; // in use
			sht->height = -1; // is hidden
			return sht;
		}
	}
	return NULL; // all sheet is occupied
}

void sheet_setbuf(SHEET *sht, uint8_t *buf, int32_t xsize, int32_t ysize, int32_t col_inv)
{
	sht->buf = buf;
	sht->bxsize = xsize;
	sht->bysize = ysize;
	sht->col_inv = col_inv;
	return;
}
