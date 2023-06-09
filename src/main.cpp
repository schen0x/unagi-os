#include "frame_buffer_config.hpp"
#include "util/kutil.h"
#include <cstdint>
// #@@range_begin(write_pixel)
struct PixelColor {
  uint8_t r, g, b;
};
int WritePixel(const FrameBufferConfig &config, int x, int y,
               const PixelColor &c);

/** Write 1 pixel
 * @retval 0		success
 * @retval non 0 	fail
 */
int WritePixel(const FrameBufferConfig &config, int x, int y,
               const PixelColor &c) {
  const int pixel_position = config.pixels_per_scan_line * y + x;
  if (config.pixel_format == kPixelRGBResv8BitPerColor) {
    uint8_t *p = (uint8_t *)(config.frame_buffer_base + 4 * pixel_position);
    p[0] = c.r;
    p[1] = c.g;
    p[2] = c.b;
  } else if (config.pixel_format == kPixelBGRResv8BitPerColor) {
    uint8_t *p = (uint8_t *)(config.frame_buffer_base + 4 * pixel_position);
    p[0] = c.b;
    p[1] = c.g;
    p[2] = c.r;
  } else {
    return -1;
  }
  return 0;
}
// #@@range_end(write_pixel)

static void PlotPixel_32bpp(int x, int y, uint32_t pixel,
                            uintptr_t frame_buffer_base, uint64_t ppl) {
  uint32_t *fb = reinterpret_cast<uint32_t *>(frame_buffer_base);
  *(fb + ppl * y + x) = pixel;
}
/**
 * or EFIAPI; Since the KernelMain is called in the UEFI Main.c
 * So make sure the KernelMain is called using the same calling convention
 * (https://gcc.gnu.org/onlinedocs/gcc/x86-Function-Attributes.html)
 * (https://clang.llvm.org/docs/AttributeReference.html#ms-abi)
 */
extern "C" void __attribute__((sysv_abi))
KernelMain(const FrameBufferConfig &frameBufferConfig) {
  //  const FrameBufferConfig fbc = {frameBufferConfig.frame_buffer_base,
  //                                 frameBufferConfig.pixels_per_scan_line,
  //                                 frameBufferConfig.horizontal_resolution,
  //                                 frameBufferConfig.vertical_resolution,
  //                                 frameBufferConfig.pixel_format};
  /* 1366x768, ppl 1366, pxFmt 1 */
  int ppl = frameBufferConfig.pixels_per_scan_line;
  int h = frameBufferConfig.vertical_resolution;
  uint32_t pixel = 0x00003200;
  for (int y = 0; y < h; y++) {
    for (int x = 0; x < ppl; x++) {
      // asm("nop");
      PlotPixel_32bpp(x, y, pixel, frameBufferConfig.frame_buffer_base, ppl);
      // WritePixel(frameBufferConfig, x, y, {255, 255, 255});
    }
  }
  asm("hlt");
  return;
}
