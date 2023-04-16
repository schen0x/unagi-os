#ifndef DRIVERS_GRATHIC_COLORTEXTMODE_H_
#define DRIVERS_GRATHIC_COLORTEXTMODE_H_

#include <stdint.h>
#include <stddef.h>
void colortextmode_terminal_initialize();
void colortextmode_terminal_put_char(const int64_t x, const int64_t y, const char c, const uint8_t color);
void colortextmode_kprint(const char* str, const size_t len, const uint8_t color);
void colortextmode_kfprint(const char* str, const uint8_t color);

#endif
