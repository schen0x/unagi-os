#ifndef UTIL_KUTIL_H_
#define UTIL_KUTIL_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
bool test_kutil();
void* kstrcpy(char* dest, const char* src);
size_t kstrlen(const char *str);
size_t kstrnlen(const char *str, size_t max);
void* kmemcpy(void* dst, const void* src, size_t size);
void* hex_to_ascii(char* ascii_str_buf, void* hex_number,  size_t size);
bool is_digit(char c);
int32_t to_digit(char c);
int32_t kmemcmp(const void *str1, const void *str2, size_t n);
void memset(void* ptr, int c, size_t size);
bool isMaskBitsAllSet (uint32_t data, uint32_t mask);
bool isMaskBitsAllClear (uint32_t data, uint32_t mask);

#endif
