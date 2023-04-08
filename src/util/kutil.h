#ifndef UTIL_KUTIL_H_
#define UTIL_KUTIL_H_

#include <stddef.h>
#include <stdint.h>
void* kstrcpy(char* dest, const char* src);
size_t kstrlen(const char *str);
void* hex_to_ascii(void* hex_ptr, char* ascii_str, size_t size);

#endif
