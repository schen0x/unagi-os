#pragma once

#include <stdint.h>

enum PixelFormat {
  kPixelRGBResv8BitPerColor,
  kPixelBGRResv8BitPerColor,
  kNotImplemented
};

typedef struct FrameBufferConfig {
  uint8_t *frame_buffer_base;
  uint32_t pixels_per_scan_line;
  uint32_t horizontal_resolution;
  uint32_t vertical_resolution;
  enum PixelFormat pixel_format;
} FrameBufferConfig;
