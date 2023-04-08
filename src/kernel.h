#ifndef KERNEL_H_
#define KERNEL_H_

#include <stdint.h>
#include <stddef.h>

void kernel_main();
void terminal_initialize();
uint16_t terminal_make_char(const char c, const uint8_t color);
void terminal_put_char(const int64_t x, const int64_t y, const char c, const uint8_t color);
void terminal_write_char(const char c, const uint8_t color);
void kprint(const char* str, const size_t len, const uint8_t color);
void kfprint(const char* str, const uint8_t color);

#endif
