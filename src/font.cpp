/**
 * @file font.cpp
 *
 * Font
 */

#include "font.hpp"
#include "./font/hankaku.h"
#include <cstdint>

/**
 * For import from objcopy binary object
 * extern const uint8_t _binary_hankaku_bin_start;
 * extern const uint8_t _binary_hankaku_bin_end;
 * extern const uint8_t _binary_hankaku_bin_size;
 */

/**
 * Get the start pointer to the character in the font object/array
 * The Hankaku font is 16 bytes per character
 * (8pixel(1Byte) per row * 16 lines)
 * 16 * 256(ASCII) == 4096 Bytes
 */
const uint8_t *GetFont(char c)
{

  auto offset = 16 * static_cast<unsigned int>(c);
  if (offset >= reinterpret_cast<uintptr_t>(sizeof(hankaku)))
  {
    return nullptr;
  }
  return hankaku + offset;
}

void WriteAscii(PixelWriter &writer, int x, int y, char c, const PixelColor &color)
{
  const uint8_t *font = GetFont(c);
  for (int dy = 0; dy < 16; ++dy)
  {
    for (int dx = 0; dx < 8; ++dx)
    {
      if ((font[dy] << dx) & 0x80u)
      {
        writer.Write(x + dx, y + dy, color);
      }
    }
  }
}

void WriteString(PixelWriter &writer, int x, int y, const char *s, const PixelColor &color)
{
  for (int i = 0; s[i] != '\0'; ++i)
  {
    WriteAscii(writer, x + 8 * i, y, s[i], color);
  }
}
