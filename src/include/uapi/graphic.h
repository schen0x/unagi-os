#ifndef INCLUDE_UAPI_GRAPHIC_H_
#define INCLUDE_UAPI_GRAPHIC_H_

#include "drivers/graphic/videomode.h"
#include "include/uapi/bootinfo.h"
#include <stddef.h>
#include <stdint.h>

void graphic_init(BOOTINFO *bi);
SHTCTL *graphic_window_manager_init(BOOTINFO *bi);

void putchar(const int64_t x, const int64_t y, const char c,
             const uint8_t color);
void kfprint(const char *str, const uint8_t color);
#endif
