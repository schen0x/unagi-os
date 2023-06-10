#include <cstdint>

#include "frame_buffer_config.hpp"
#include "util/kutil.h"

FrameBufferConfig frameBufferConfig = {0}; // .bss RW

// #@@range_begin(write_pixel)
struct PixelColor
{
  uint8_t r, g, b;
};

/** Write 1 pixel
 * @retval 0		success
 * @retval non 0 	fail
 */
int WritePixel(const FrameBufferConfig &config, int x, int y, const PixelColor &c)
{
  const int pixel_position = config.pixels_per_scan_line * y + x;
  if (config.pixel_format == kPixelRGBResv8BitPerColor)
  {
    uint8_t *p = (uint8_t *)(config.frame_buffer_base + 4 * pixel_position);
    p[0] = c.r;
    p[1] = c.g;
    p[2] = c.b;
  }
  else if (config.pixel_format == kPixelBGRResv8BitPerColor)
  {
    uint8_t *p = (uint8_t *)(config.frame_buffer_base + 4 * pixel_position);
    p[0] = c.b;
    p[1] = c.g;
    p[2] = c.r;
  }
  else
  {
    return -1;
  }
  return 0;
}
// #@@range_end(write_pixel)

static void PlotPixel_32bpp(int x, int y, uint32_t pixel, uintptr_t frame_buffer_base, uint64_t ppl)
{
  uint32_t *fb = reinterpret_cast<uint32_t *>(frame_buffer_base);
  *(fb + ppl * y + x) = pixel;
}

// #@@range_begin(placement_new)
void *operator new(size_t size, void *buf)
{
  return buf;
}

void operator delete(void *obj) noexcept
{
}
// #@@range_end(placement_new)

// #@@range_begin(pixel_writer)
class PixelWriter
{
public:
  PixelWriter(const FrameBufferConfig &config) : config_{config}
  {
  }
  virtual ~PixelWriter() = default;
  virtual void Write(int x, int y, const PixelColor &c) = 0;

protected:
  uint8_t *PixelAt(int x, int y)
  {
    return (uint8_t *)config_.frame_buffer_base + 4 * (config_.pixels_per_scan_line * y + x);
  }

private:
  const FrameBufferConfig &config_;
};
// #@@range_end(pixel_writer)

// #@@range_begin(derived_pixel_writer)
class RGBResv8BitPerColorPixelWriter : public PixelWriter
{
public:
  using PixelWriter::PixelWriter;

  virtual void Write(int x, int y, const PixelColor &c) override
  {
    auto p = PixelAt(x, y);
    p[0] = c.r;
    p[1] = c.g;
    p[2] = c.b;
  }
};

class BGRResv8BitPerColorPixelWriter : public PixelWriter
{
public:
  using PixelWriter::PixelWriter;

  virtual void Write(int x, int y, const PixelColor &c) override
  {
    auto p = PixelAt(x, y);
    p[0] = c.b;
    p[1] = c.g;
    p[2] = c.r;
  }
};
// #@@range_end(derived_pixel_writer)

char pixel_writer_buf[sizeof(RGBResv8BitPerColorPixelWriter)];
PixelWriter *pixel_writer;

/**
 * sysv_abi, ms_abi or whatever calling convention,
 * the callee and the caller must use the same one.
 * (https://gcc.gnu.org/onlinedocs/gcc/x86-Function-Attributes.html)
 * (https://clang.llvm.org/docs/AttributeReference.html#ms-abi)
 */
extern "C" void __attribute__((sysv_abi)) KernelMain(const FrameBufferConfig &__frameBufferConfig)
{
  /* Store the param into our managed address */
  frameBufferConfig = __frameBufferConfig;

  switch (frameBufferConfig.pixel_format)
  {
  case kPixelRGBResv8BitPerColor:
    pixel_writer = new (pixel_writer_buf) RGBResv8BitPerColorPixelWriter{frameBufferConfig};
    break;
  case kPixelBGRResv8BitPerColor:
    pixel_writer = new (pixel_writer_buf) BGRResv8BitPerColorPixelWriter{frameBufferConfig};
    break;
  }
  for (int y = 0; y < frameBufferConfig.vertical_resolution; y++)
  {
    for (int x = 0; x < frameBufferConfig.horizontal_resolution; x++)
    {
      pixel_writer->Write(x, y, {255, 255, 255});
    }
  }
  for (int y = 0; y < 100; y++)
  {
    for (int x = 0; x < 200; x++)
    {
      pixel_writer->Write(x, y, {0, 255, 0});
      // PlotPixel_32bpp(x, y, 0x12800, frameBufferConfig.frame_buffer_base, frameBufferConfig.pixels_per_scan_line);
    }
  }
  asm("hlt");
  return;
}
