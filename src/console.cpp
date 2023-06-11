/**
 * @file console.cpp
 *
 * Console
 */

#include "console.hpp"
#include "config.hpp"
#include "font.hpp"
#include <cstring>

/* Constructors and member initializer lists */
Console::Console(PixelWriter &writer, const PixelColor &fg_color, const PixelColor &bg_color)
    : writer_{writer}, fg_color_{fg_color}, bg_color_{bg_color}, buffer_{}, cursor_row_{0}, cursor_column_{0}
{
}

void Console::PutString(const char *s)
{
  int __i = 0;
  while (*s && ++__i < SYS_MAX_ITER)
  {
    if (*s == '\n')
    {
      Newline();
    }
    else if (cursor_column_ < kColumns - 1)
    {
      WriteAscii(writer_, 8 * cursor_column_, 16 * cursor_row_, *s, fg_color_);
      buffer_[cursor_row_][cursor_column_] = *s;
      ++cursor_column_;
    }
    ++s;
  }
}

/**
 * If overflow:
 *   - Clear the screen
 *   - Move row to prev row in buffer_
 *   - Write the new row
 */
void Console::Newline()
{
  cursor_column_ = 0;
  if (cursor_row_ < kRows - 1)
  {
    ++cursor_row_;
  }
  else
  {
    /* Clear the screen */
    for (int y = 0; y < 16 * kRows; ++y)
    {
      for (int x = 0; x < 8 * kColumns; ++x)
      {
        writer_.Write(x, y, bg_color_);
      }
    }
    /* Rewrite all rows, so that contents of row == ++row */
    for (int row = 0; row < kRows - 1; ++row)
    {
      memcpy(buffer_[row], buffer_[row + 1], kColumns + 1);
      /* Rewrite one row, from the start, to the EOL */
      WriteString(writer_, 0, 16 * row, buffer_[row], fg_color_);
    }
    /* Clear char buffer of the last row */
    memset(buffer_[kRows - 1], 0, kColumns + 1);
  }
}
