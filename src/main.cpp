#include "frame_buffer_config.hpp"
#include <cstdint>

static inline void PlotPixel_32bpp(int x, int y, uint32_t pixel,
                                   uint64_t frame_buffer_base, uint64_t ppl) {
  *((uint32_t *)(frame_buffer_base + 4 * ppl * y + 4 * x)) = pixel;
}
/**
 * or EFIAPI; Since the KernelMain is called in the UEFI Main.c
 * So make sure the KernelMain is called using the same calling convention
 * (https://gcc.gnu.org/onlinedocs/gcc/x86-Function-Attributes.html)
 * (https://clang.llvm.org/docs/AttributeReference.html#ms-abi)
 */
extern "C" void __attribute__((sysv_abi))
KernelMain(volatile FrameBufferConfig *frameBufferConfig) {
  /* 1366x768, ppl 1366, pxFmt 1 */
  int ppl = frameBufferConfig->pixels_per_scan_line;
  int h = frameBufferConfig->vertical_resolution;
  for (int y = 0; y < h; y++) {
    if (y % h > (h / 2))
      continue;
    for (int x = 0; x < ppl; x++) {
      PlotPixel_32bpp(
          x, y, 12800,
          reinterpret_cast<uint64_t>(frameBufferConfig->frame_buffer_base),
          ppl);
    }
  }

  return;
}
