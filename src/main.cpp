#include <cstddef>
#include <cstdint>
#include <cstdio> // use the newlib
#include <new>

#include "console.hpp"
#include "font.hpp"
#include "frame_buffer_config.hpp"
#include "graphics.hpp"
#include "pci.hpp"

FrameBufferConfig frameBufferConfig = {0}; // .bss RW
char console_buf[sizeof(Console)];         // The buffer for placement new
Console *console;
const PixelColor kDesktopBGColor{45, 118, 237};
const PixelColor kDesktopFGColor{255, 255, 255};

void operator delete(void *obj) noexcept
{
}

const int kMouseCursorWidth = 15;
const int kMouseCursorHeight = 24;
// clang-format off
const char mouse_cursor_shape[kMouseCursorHeight][kMouseCursorWidth + 1] = {
    "@              ",
    "@@             ",
    "@.@            ",
    "@..@           ",
    "@...@          ",
    "@....@         ",
    "@.....@        ",
    "@......@       ",
    "@.......@      ",
    "@........@     ",
    "@.........@    ",
    "@..........@   ",
    "@...........@  ",
    "@............@ ",
    "@......@@@@@@@@",
    "@......@       ",
    "@....@@.@      ",
    "@...@ @.@      ",
    "@..@   @.@     ",
    "@.@    @.@     ",
    "@@      @.@    ",
    "@       @.@    ",
    "         @.@   ",
    "         @@@   ",
};
// clang-format on

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

  const int kFrameWidth = frameBufferConfig.horizontal_resolution;
  const int kFrameHeight = frameBufferConfig.vertical_resolution;
  // #@@range_begin(draw_desktop)
  FillRectangle(*pixel_writer, {0, 0}, {kFrameWidth, kFrameHeight - 50}, kDesktopBGColor);
  FillRectangle(*pixel_writer, {0, kFrameHeight - 50}, {kFrameWidth, 50}, {1, 8, 17});
  FillRectangle(*pixel_writer, {0, kFrameHeight - 50}, {kFrameWidth / 5, 50}, {80, 80, 80});
  DrawRectangle(*pixel_writer, {10, kFrameHeight - 40}, {30, 30}, {160, 160, 160});

  console = new (console_buf) Console{*pixel_writer, kDesktopFGColor, kDesktopBGColor};
  printk("Welcome to MikanOS!\n");

  /**
   * Draw the cursor at (200, 100)
   */
  for (int dy = 0; dy < kMouseCursorHeight; ++dy)
  {
    for (int dx = 0; dx < kMouseCursorWidth; ++dx)
    {
      if (mouse_cursor_shape[dy][dx] == '@')
      {
        pixel_writer->Write(200 + dx, 100 + dy, {0, 0, 0});
      }
      else if (mouse_cursor_shape[dy][dx] == '.')
      {
        pixel_writer->Write(200 + dx, 100 + dy, {255, 255, 255});
      }
    }
  }
  auto err = pci::ScanAllBus();
  printk("ScanAllBus: %s\n", err.Name());

  for (int i = 0; i < pci::num_device; ++i)
  {
    const auto &dev = pci::devices[i];
    auto vendor_id = pci::ReadVendorId(dev.bus, dev.device, dev.function);
    auto class_code = pci::ReadClassCode(dev.bus, dev.device, dev.function);
    printk("%d.%d.%d: vend %04x, class %08x, head %02x\n", dev.bus, dev.device, dev.function, vendor_id, class_code,
           dev.header_type);
  }
  asm("hlt");
  return;
}
