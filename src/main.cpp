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
extern "C" void __attribute__((ms_abi))
KernelMain(volatile uint64_t frame_buffer_base,
           volatile uint64_t frame_buffer_size) {
  /* 1366x768, ppl 1366, pxFmt 1 */
  int w = 1366;
  int h = 768;
  for (int y = 0; y < h; y++) {
    if (y % h > (h / 2))
      continue;
    for (int x = 0; x < w; x++) {
      PlotPixel_32bpp(x, y, 12800, frame_buffer_base, w);
    }
  }

  // frame_buffer_base = 0x80000000;
  // frame_buffer_size = 0x1d5000;
  // uint8_t *frame_buffer = reinterpret_cast<uint8_t *>(frame_buffer_base);
  //  for (uint64_t i = 0; i < frame_buffer_size; i++) {
  //    frame_buffer[i] = i % 256;
  //  }

  return;
}
