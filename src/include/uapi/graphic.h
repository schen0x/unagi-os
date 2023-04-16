#ifndef INCLUDE_UAPI_GRAPHIC_H_
#define INCLUDE_UAPI_GRAPHIC_H_

#include <stdint.h>
#include <stddef.h>

void graphic_initialize();
void putchar(const int64_t x, const int64_t y, const char c, const uint8_t color);
void kfprint(const char* str, const uint8_t color);
#endif
