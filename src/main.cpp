#include <cstddef>
#include <cstdint>
#include <cstdio> // use the newlib
#include <new>

#include "console.hpp"
#include "font.hpp"
#include "frame_buffer_config.hpp"
#include "graphics.hpp"
#include "logger.hpp"
#include "mouse.hpp"
#include "pci.hpp"
#include "usb/classdriver/mouse.hpp"
#include "usb/device.hpp"
#include "usb/memory.hpp"
#include "usb/xhci/trb.hpp"
#include "usb/xhci/xhci.hpp"

FrameBufferConfig frameBufferConfig = {}; // .bss RW, {0} is C syntax, does not work in cpp
char console_buf[sizeof(Console)];        // The buffer for placement new
Console *console;
const PixelColor kDesktopBGColor{45, 118, 237};
const PixelColor kDesktopFGColor{255, 255, 255};

void operator delete(void *obj) noexcept
{
  (void)obj;
}

char mouse_cursor_buf[sizeof(MouseCursor)];
MouseCursor *mouse_cursor;

void MouseObserver(uint8_t buttons, int8_t displacement_x, int8_t displacement_y)
{
  mouse_cursor->MoveRelative({displacement_x, displacement_y});
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
 * Enable XHCI (USB3.0 controller) if device is Intel 7 Series PCH
 * Intel 7 Series / C216 Chipset Family Platform Controller Hub (PCH) - Datasheet
 * - By default EHCI (USB2.0) is enabled
 * - Switch to XHCI (USB3)
 */
void SwitchEhci2Xhci(const pci::Device &xhc_dev)
{
  bool intel_ehc_exist = false;
  for (int i = 0; i < pci::num_device; ++i)
  {
    if (pci::devices[i].class_code.Match(0x0cu, 0x03u, 0x20u) /* EHCI */ &&
        0x8086 == pci::ReadVendorId(pci::devices[i]))
    {
      intel_ehc_exist = true;
      break;
    }
  }
  if (!intel_ehc_exist)
  {
    return;
  }

  /**
   * USB3PRM—USB 3.0 Port Routing Mask Register
   * bit 3:0 is the USB 2.0 Host Controller Selector Mask (USB2HCSELM) — R/W.
   * This bit field allows the BIOS to communicate to the OS which USB 3.0
   * ports can have the SuperSpeed capabilities enabled.
   */
  uint32_t superspeed_ports = pci::ReadConfReg(xhc_dev, 0xdc);
  /**
   * USB3_PSSEN—USB 3.0 Port SuperSpeed Enable Register
   * bit 3:0 is the USB 3.0 Port SuperSpeed Enable(USB3_PSSEN) — R/W.
   * This field controls whether SuperSpeed capability is enabled for a given
   * USB 3.0 port.
   */
  pci::WriteConfReg(xhc_dev, 0xd8, superspeed_ports);
  /**
   * XUSB2PRM—xHC USB 2.0 Port Routing Mask Register
   * bit 3:0 is the USB 2.0 Host Controller Selector Mask (USB2HCSELM) — R/W.
   * This bit field allows the BIOS to communicate to the OS which USB 2.0
   * ports can be switched from the EHC controller to the xHC controller.
   */
  uint32_t ehci2xhci_ports = pci::ReadConfReg(xhc_dev, 0xd4);
  /**
   * XUSB2PR —xHC USB 2.0 Port Routing Register
   * bit 3:0 is the USB 2.0 Host Controller Selector (USB2HCSEL) — R/W.
   * Maps a USB 2.0 port to the xHC or EHC #1 host controller.
   */
  pci::WriteConfReg(xhc_dev, 0xd0, ehci2xhci_ports);
  Log(kDebug, "SwitchEhci2Xhci: SS = %02x, xHCI = %02x\n", superspeed_ports, ehci2xhci_ports);
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
  printk("Unagi!\n");
  SetLogLevel(kDebug);

  /**
   * Draw the cursor
   */
  mouse_cursor = new (mouse_cursor_buf) MouseCursor{pixel_writer, kDesktopBGColor, {600, 800}};
  auto err = pci::ScanAllBus();
  Log(kDebug, "ScanAllBus: %s\n", err.Name());

  for (int i = 0; i < pci::num_device; ++i)
  {
    const auto &dev = pci::devices[i];
    auto vendor_id = pci::ReadVendorId(dev.bus, dev.device, dev.function);
    auto class_code = pci::ReadClassCode(dev.bus, dev.device, dev.function);
    Log(kDebug, "%d.%d.%d: vend %04x, class %08x, head %02x\n", dev.bus, dev.device, dev.function, vendor_id,
        class_code, dev.header_type);
  }

  /**
   * Search through the pci devices, find the first (if Intel) or the last USB3 Controller
   * - 0xC - Serial Bus Controller
   * - 0x3 - USB Controller
   * - 0x30 - XHCI (USB3) Controller
   * - VendorId 0x8086 Intel Corporation
   */
  pci::Device *xhc_dev = nullptr;
  for (int i = 0; i < pci::num_device; ++i)
  {
    if (pci::devices[i].class_code.Match(0x0cu, 0x03u, 0x30u))
    {
      xhc_dev = &pci::devices[i];

      if (0x8086 == pci::ReadVendorId(*xhc_dev))
      {
        break;
      }
    }
  }

  /**
   * Use the Intel (Extensible Host Controller) XHC
   */
  if (xhc_dev)
  {
    Log(kInfo, "xHC (Intel) has been found: %d.%d.%d\n", xhc_dev->bus, xhc_dev->device, xhc_dev->function);

    /* Read BAR and find the Memory-mapped I/O (MMIO) address */
    const WithError<uint64_t> xhc_bar = pci::ReadBar(*xhc_dev, 0);
    Log(kDebug, "ReadBar: %s\n", xhc_bar.error.Name());
    /* MMIO address = BAR with the lower 4 bits (flags) masked */
    const uint64_t xhc_mmio_base = xhc_bar.value & ~static_cast<uint64_t>(0xf);
    Log(kDebug, "xHC mmio_base = %08lx\n", xhc_mmio_base);

    /* Initialize the xhc, with the MMIO address */
    usb::xhci::Controller xhc{xhc_mmio_base};

    if (0x8086 == pci::ReadVendorId(*xhc_dev))
    {
      SwitchEhci2Xhci(*xhc_dev);
    }

    {
      auto err = xhc.Initialize();
      Log(kDebug, "xhc.Initialize: %s\n", err.Name());
    }

    Log(kInfo, "xHC starting\n");
    xhc.Run();
    usb::HIDMouseDriver::default_observer = MouseObserver;

    for (int i = 1; i <= xhc.MaxPorts(); ++i)
    {
      auto port = xhc.PortAt(i);
      Log(kDebug, "Port %d: IsConnected=%d\n", i, port.IsConnected());

      if (port.IsConnected())
      {
        if (auto err = ConfigurePort(xhc, port))
        {
          Log(kError, "failed to configure port: %s at %s:%d\n", err.Name(), err.File(), err.Line());
          continue;
        }
      }
    }
  }

  asm("hlt");
  return;
}
