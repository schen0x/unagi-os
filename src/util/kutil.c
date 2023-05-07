#include "util/kutil.h"
#include "config.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
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

/*
 * This function is required by gcc compiler
 * Probably when dynamic sized initialization is asked in the stack.
 * e.g.: struct PATH_PART* pt_ptr[OS_PATH_MAX_LENGTH] = {0}; // (a line in the stack)
 * */
void memset(void* ptr, int c, size_t size)
{
	kmemset(ptr, c, size);
	return;
}


/* Fill "*ptr" with (char)"c" * "size" */
void* kmemset(void* ptr, int c, size_t size)
{
	/* write size * c to (*ptr) */
	char* dst = ptr;
//	while (size--)
//		*dst++ = c;
	for(size_t i=0; i < size; i++)
	{
		dst[i] = (char) c;
	}
	return ptr;
}


void* kmemcpy(void* dst, const void* src, size_t size)
{
	char* c_dst = dst;
	const char* c_src = src;
	for(size_t i=0; i < size; i++)
	{
		c_dst[i] = c_src[i];
	}
	return dst;
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

bool isMaskBitsAllSet (uint32_t data, uint32_t mask)
{
	if ((data & mask) == mask)
		return true;
	return false;
}

bool isMaskBitsAllClear (uint32_t data, uint32_t mask)
{
	if ((data & mask) == 0)
		return true;
	return false;
}
/*
 * Return the closest upper value that align.
 * @addr memory address
 * @ALIGN in bytes; e.g., 16, 4096
 * e.g., 50->4096; 4097 -> 8192;
 */
uintptr_t align_address_to_upper(uintptr_t addr, uint32_t ALIGN)
{
	uintptr_t residual = addr % ALIGN;
    	if (residual == 0)
    	        return addr;
	addr += (ALIGN - residual); // ALIGN >= residual
    	return addr;
}

/*
 * Return the closest lower value that align.
 * @addr memory address
 * @ALIGN in bytes; e.g., 16, 4096
 * e.g., 4097 -> 4096; 8193 -> 8192;
 */
uintptr_t align_address_to_lower(uintptr_t addr, uint32_t ALIGN)
{
	uintptr_t residual = addr % ALIGN;
    	if (residual == 0)
    	{
    	        return addr;
    	}
    	addr -= residual; // val >= residual
    	return addr;
}


/**
 * Remove element by indexes in `index_to_remove`, from the array `arr`
 * Assume `index_to_remove` is sorted in ascending order.
 * e.g. int arr[256] = {10, 20, 30, 40, 50, 60};
 * index_to_remove[128] = {1, 3};
 * Result: arr == {10, 30, 50, 60, 0, 0, ...};
 */
void arr_remove_element_u32(uint32_t arr[], uint32_t index_to_remove[], uint32_t arr_size, uint32_t index_to_remove_size)
{
	uint32_t oldi = 0, di = 0, newi = 0;

	for (oldi = 0; oldi < arr_size; oldi++)
	{
		if (di < index_to_remove_size && oldi == index_to_remove[di])
		{
			di++;
			continue;
		}
		arr[newi++] = arr[oldi];
	}
	return;
}


bool test_kutil()
{
	if (isMaskBitsAllSet(0b10111111, 0b1010) != true)
		return false;
	if (isMaskBitsAllSet(0b10111111, 0b11111111) != false)
		return false;
	if (isMaskBitsAllSet(0b10111111, 0b11011111) != false)
		return false;
	if (isMaskBitsAllClear(0b10111111, 0b01000000) != true)
		return false;
	if (isMaskBitsAllClear(0b1001, 0b0110) != true)
		return false;
	uint32_t arr[128] = {10, 20, 30, 40, 50, 60, 70, 80};
	uint32_t index_to_remove[128] = {2, 4};
	uint32_t arr_size = 8;
	uint32_t index_size = 2;
	arr_remove_element_u32(arr, index_to_remove, arr_size, index_size);
	uint32_t expected_arr[6] = {10, 20, 40, 60, 70, 80};
	for (int i = 0; i < 6; i++)
	{
		if (arr[i] != expected_arr[i])
			return false;
	}
	return true;
}

