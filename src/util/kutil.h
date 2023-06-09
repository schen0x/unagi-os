#ifndef UTIL_KUTIL_H_
#define UTIL_KUTIL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
bool test_kutil();
void *kstrcpy(char *dest, const char *src);
size_t kstrlen(const char *str);
size_t kstrnlen(const char *str, size_t max);
void *kmemcpy(void *dst, const void *src, size_t size);
void *hex_to_ascii(char *ascii_str_buf, void *hex_number, size_t size);
bool is_digit(char c);
int32_t to_digit(char c);
int32_t kmemcmp(const void *str1, const void *str2, size_t n);
void memset(void *ptr, int c, size_t size);
void *kmemset(void *ptr, int c, size_t size);
bool isMaskBitsAllSet(uint32_t data, uint32_t mask);
bool isMaskBitsAllClear(uint32_t data, uint32_t mask);
uintptr_t align_address_to_upper(uintptr_t addr, uint32_t ALIGN);
uintptr_t align_address_to_lower(uintptr_t val, uint32_t ALIGN);
void arr_remove_element_u32(uint32_t arr[], uint32_t index_to_remove[],
                            uint32_t arr_size, uint32_t index_to_remove_size);
int32_t kstrcmp(const unsigned char *p1, const unsigned char *p2);
#ifdef __cplusplus
}
#endif
#endif
