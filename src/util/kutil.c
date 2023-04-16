#include "util/kutil.h"
#include "config.h"
#include <stdint.h>
#include <stdbool.h>
#include "memory/memory.h"

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

size_t kstrnlen(const char *str, size_t max)
{
	size_t len = 0;
	for (len = 0; len < max; len++)
	{
		if(str[len] == '\0')
			break;
	}
	return len;
}

void memset(void* ptr, int c, size_t size)
{
	kmemset(ptr, c, size);
}


/*
 * Write the ascii representation of hex_number at &hex_number.
 * Usage: kfprint((hex_to_ascii(buf, &hex, sizeof(hex)))),4);
 */
void* hex_to_ascii(char* ascii_str_buf, void* hex_number,  size_t size)
{
    uint8_t* bytes = (uint8_t*)hex_number;
    const char hex_chars[] = "0123456789abcdef";

    for (int i = size - 1; i >= 0; i--)
    {
        uint8_t byte = bytes[i];
        ascii_str_buf[(size - i - 1) * 2] = hex_chars[(byte >> 4) & 0xF];
        ascii_str_buf[(size - i - 1) * 2 + 1] = hex_chars[byte & 0xF];
    }

    ascii_str_buf[size * 2] = '\0';
    return ascii_str_buf;
}

/* Check if char is ascii '0' - '9' */
bool is_digit(char c)
{
	return c >= 48 && c <= 57; // true if ascii 0-9
}

/* Convert ascii char '0' - '9' to digit */
int32_t to_digit(char c)
{
	return c - 48;
}


/*
 * COPY_AND_PASTED
 * The gcc implemtation of memcmp.c
 *
 * memcmp -- compare two memory regions.
 * This function is in the public domain.
 *
 * @deftypefn Supplemental int memcmp (const void *@var{x}, const void *@var{y}, @
 *   size_t @var{count})
 * Compares the first @var{count} bytes of two areas of memory.  Returns
 * zero if they are the same, a value less than zero if @var{x} is
 * lexically less than @var{y}, or a value greater than zero if @var{x}
 * is lexically greater than @var{y}.  Note that lexical order is determined
 * as if comparing unsigned char arrays.
 * @end deftypefn
*/
int32_t kmemcmp(const void *str1, const void *str2, size_t n)
{
	register const unsigned char *s1 = (const unsigned char*)str1;
	register const unsigned char *s2 = (const unsigned char*)str2;

	while (n-- > 0)
	  {
	    if (*s1++ != *s2++)
	        return s1[-1] < s2[-1] ? -1 : 1;
	  }
	return 0;
}
