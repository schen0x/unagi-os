#ifndef INCLUDE_UAPI_BOOTINFO_H_
#define INCLUDE_UAPI_BOOTINFO_H_

#include <stdint.h>
typedef struct BOOTINFO
{
	uint8_t cyls, leds, vmode, reserve;
	int16_t scrnx, scrny;
	uint32_t *vram; // uintptr_t
} BOOTINFO;

#endif
