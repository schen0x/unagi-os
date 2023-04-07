#include "util/kutil.h"
#include "config.h"

void* kstrcpy(char* dest, const char* src)
{
	while (*src)
	{
		*dest++ = *src++;
	}
	*dest = '\0';
	return dest;
}

size_t kstrlen(const char *str)
{
	size_t len = 0;
	while (str[len] != '\0')
	{
		len++;
	}
	return len;
}

void* hex_to_ascii(void* hex_number, char* ascii_str, size_t size)
{
    uint8_t* bytes = (uint8_t*)hex_number;
    const char hex_chars[] = "0123456789ABCDEF";

    for (int i = size - 1; i >= 0; i--)
    {
        uint8_t byte = bytes[i];
        ascii_str[(size - i - 1) * 2] = hex_chars[(byte >> 4) & 0xF];
        ascii_str[(size - i - 1) * 2 + 1] = hex_chars[byte & 0xF];
    }

    ascii_str[size * 2] = '\0';
    return ascii_str;
}
