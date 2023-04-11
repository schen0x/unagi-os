#ifndef UTIL_KUTIL_H_
#define UTIL_KUTIL_H_

#include <stddef.h>
#include <stdint.h>
void* kstrcpy(char* dest, const char* src);
size_t kstrlen(const char *str);
void* hex_to_ascii(char* ascii_str_buf, void* hex_number,  size_t size);

#endif
