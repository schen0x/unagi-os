#include <cstddef>
#include <cstdint>
#include <cstdio> // use the newlib

#include "console.hpp"
#include "font.hpp"
#include "frame_buffer_config.hpp"
#include "graphics.hpp"

FrameBufferConfig frameBufferConfig = {0}; // .bss RW
char console_buf[sizeof(Console)];         // The buffer for placement new
Console *console;

void *operator new(size_t size, void *buf)
{
  return buf;
}

void operator delete(void *obj) noexcept
{
}

char pixel_writer_buf[sizeof(RGBResv8BitPerColorPixelWriter)];
PixelWriter *pixel_writer;

int printk(const char *format, ...)
{
  va_list ap;
  int result;
  char s[1024];

  va_start(ap, format);
  result = vsprintf(s, format, ap);
  va_end(ap);

  console->PutString(s);
  return result;
}

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
  console = new (console_buf) Console{*pixel_writer, {0, 0, 0}, {255, 255, 255}};

  int i = 0;
  for (char c = '!'; c <= '~'; ++c, ++i)
  {
    printk("printk: %d\n", i);
  }
  asm("hlt");
  return;
}
