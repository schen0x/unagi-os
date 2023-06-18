#pragma once

#include "graphics.hpp"

class Console
{
public:
  static const int kRows = 55, kColumns = 80;

  Console(PixelWriter &writer, const PixelColor &fg_color, const PixelColor &bg_color);
  void PutString(const char *s);

private:
  void Newline();

  PixelWriter &writer_;
  const PixelColor fg_color_, bg_color_;
  /* The ASCII value/char of all characters in the console */
  char buffer_[kRows][kColumns + 1];
  int cursor_row_, cursor_column_;
};
