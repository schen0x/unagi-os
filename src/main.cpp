#include <cstddef>
#include <cstdint>
#include <cstdio> // use the newlib
#include <new>

#include "asmfunc.h"
#include "console.hpp"
#include "font.hpp"
#include "frame_buffer_config.hpp"
#include "graphics.hpp"
#include "interrupt.hpp"
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

void MouseObserver(int8_t displacement_x, int8_t displacement_y)
{
  Log(kDebug, "--**MouseObserver: %d, %d", displacement_x, displacement_y);
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

// #@@range_begin(xhci_handler)
usb::xhci::Controller *xhc;
uint8_t __buf_xhc[sizeof(usb::xhci::Controller)];

/**
 * __attribute__((interrupt)):
 *   - https://releases.llvm.org/5.0.1/tools/clang/docs/AttributeReference.html#interrupt
 *   - Clang supports the GNU style __attribute__((interrupt)) attribute on
 *   x86/x86-64 targets.The compiler generates function entry and exit
 *   sequences suitable for use in an interrupt handler when this attribute is
 *   present. The ‘IRET’ instruction, instead of the ‘RET’ instruction, is used
 *   to return from interrupt or exception handlers. All registers, except for
 *   the EFLAGS register which is restored by the ‘IRET’ instruction, are
 *   preserved by the compiler. Any interruptible-without-stack-switch code
 *   must be compiled with -mno-red-zone since interrupt handlers can and will,
 *   because of the hardware design, touch the red zone.
 *
 */
__attribute__((interrupt)) void IntHandlerXHCI(InterruptFrame *frame)
{
  (void)frame;
  while (xhc->PrimaryEventRing()->HasFront())
  {
    if (auto err = ProcessEvent(*xhc))
    {
      Log(kError, "Error while ProcessEvent: %s at %s:%d\n", err.Name(), err.File(), err.Line());
    }
  }
  NotifyEndOfInterrupt();
}
// #@@range_end(xhci_handler)

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

    // #@@range_begin(load_idt)
    /**
     * Add the xHCI INT vector (0x40) to IDT
     * Use the current CodeSegment Selector
     */
    const uint16_t cs = GetCS();
    SetIDTEntry(idt[InterruptVector::kXHCI], MakeIDTAttr(DescriptorType::kInterruptGate, 0),
                reinterpret_cast<uint64_t>(IntHandlerXHCI), cs);
    /**
     * @limit sizeof(idt) - 1
     * @offset &idt[0]
     */
    LoadIDT(sizeof(idt) - 1, reinterpret_cast<uintptr_t>(&idt[0]));
    // #@@range_end(load_idt)

    // #@@range_begin(configure_msi)
    /**
     * Read the Local APIC ID from the "0xFEE0 0020H (Local APIC ID Register)"
     *
     * Intel 64 Software Developer's Manual Vol.3A
     *   - Table 11-1. Local APIC Register Address Map (1-4, p3389)
     *   - 9.4.3 MP Initialization Protocol Algorithm for MP Systems (1-4, p3296)
     */
    const uint8_t bsp_local_apic_id = *reinterpret_cast<const uint32_t *>(0xfee00020) >> 24;
    pci::ConfigureMSIFixedDestination(*xhc_dev, bsp_local_apic_id, pci::MSITriggerMode::kLevel,
                                      pci::MSIDeliveryMode::kFixed, InterruptVector::kXHCI, 0);
    // #@@range_end(configure_msi)

    /* Read BAR and find the Memory-mapped I/O (MMIO) address */
    const WithError<uint64_t> xhc_bar = pci::ReadBar(*xhc_dev, 0);
    Log(kDebug, "ReadBar: %s\n", xhc_bar.error.Name());
    /* MMIO address = BAR with the lower 4 bits (flags) masked */
    const uint64_t xhc_mmio_base = xhc_bar.value & ~static_cast<uint64_t>(0xf);
    Log(kDebug, "xHC mmio_base = %08lx\n", xhc_mmio_base);

    /**
     * xHCI spec 3.1: xHCI defines three interface spaces:
     *   - Host Configuration Space (PCI Config Space)
     *   - MMIO Space
     *     - Capability Registers (specify read-only limits, restrictions and
     *     capabilities of the host controller implementation. These values are
     *     used as parameters to the host controller driver.)
     *     - Operational Registers (Accessed during Init; `Cmd Ring` + `Device
     *     Context Base Address Array` (DCBAA[slot_id].`Device Contexts`)->`Transfer
     *     Ring`->`Data Buffer`)
     *     - xHCI Extended Capabilities (specify optional features of an xHC
     *     implementation, as well as providing the ability to add new
     *     capabilities to implementations after the publication of this
     *     specification.)
     *     - Runtime Registers (Heavily accessed during Runtime; specify `Event
     *     Ring Segment Table`->`Event Ring`)
     *     - Doorbell(DB) Array (Support max 256 `Doorbell Registers`, which
     *     supports up to 255 USB devices or hubs. System software can notify
     *     the xHC if it has `Slot` or `Endpoint` related work to perform; A DB
     *     Target field in the Doorbell Register is written with a value that
     *     identifies the reason for “ringing” the doorbell. Doorbell Register
     *     0 is allocated to the Host Controller for Command Ring management.)
     *     - Host Memory
     *
     * TERMS:
     *   - Device Slot: a generic reference to a set of xHCI data structures
     *   associated with an individual USB device. Each device is represented
     *   by an entry in the Device Context Base Address Array, a register in
     *   the Doorbell Array register, and a device’s Device Context.
     *   - Slot ID: the index used to identify a specific Device Slot. For
     *   example the value of Slot ID will be used as an index to identify a
     *   specific entry in the Device Context Base Address Array.
     *   - Device Context Base Address Array (DCBAA): supports up to 255 USB devices or
     *   hubs, where each element in the array is a pointer to a Device Context
     *   data structure. The first entry DCBAA[0] is used by the `xHCI
     *   Scratchpad mechanism`. Refer to section 4.20 for more information.
     *   - Command Ring: used by software to pass device and host controller
     *   related commands to the xHC. The Command Ring shall be treated as
     *   read-only by the xHC. Refer to section 4.9.3 for a discussion of
     *   Command Ring Management.
     *   - The Event Ring is used by the xHC to pass command completion and
     *   asynchronous events to software. The Event Ring shall be treated as
     *   read -only by system software. Refer to section 4.9.4 for a discussion
     *   of Event Ring Management.
     *   - A Transfer Ring is used by software to schedule work items for a
     *   single USB Endpoint. A Transfer Ring is organized as a circular queue
     *   of Transfer Descriptor (TD) data structures, where each Transfer
     *   Descriptor defines one or more Data Buffers that will be moved to or
     *   from the USB. Transfer Rings are treated as read-only by the xHC.
     *   Refer to section 4.9.2 for a discussion of Transfer Ring Management.
     *   - All three types of rings support the ability for system software to
     *   grow or shrink them while they are active. Special TDs written to the
     *   Transfer and Command rings allow software to change their size,
     *   however since the Event Ring is read - only to software, the *Event
     *   Ring Segment Table* is provided so that software may modify its size.
     *
     *   - The Device Context data structure is managed by the xHC and used to
     *   report device configuration and state information to system software.
     *   The Device Context data structure consists of an array of 32 data
     *   structures. The first context data structure (index = ‘0’) is a Slot
     *   Context data structure (6.2.2). The remaining context data structures
     *   (indices 1-31) are Endpoint Context data structures (6.2.3).
     *   As part of the process of enumerating a USB device, system software
     *   allocates a Device Context data structure for the device in host
     *   memory and initializes it to ‘0’. Ownership of the data structure is
     *   then passed to the xHC with an Address Device Command. The xHC
     *   maintains ownership of the Device Context until the device slot is
     *   disabled with a Disable Slot Command. The Device Context data
     *   structure shall be treated as read-only by system software while it is
     *   owned by the xHC.
     *
     *   - The Slot Context data structure contains information that relates to the device as a whole, or affects all
     * endpoints of a USB device. This data structure is defined as a member of the Device Context and Input Context
     * data structures. Refer to section 3.2.5 for information on the Input Context data structure.
     *
     * - Endpoint Context: The Endpoint Context data structure defines the configuration and state of a specific USB
     * endpoint.
     */
    xhc = new (__buf_xhc) usb::xhci::Controller(xhc_mmio_base);

    if (0x8086 == pci::ReadVendorId(*xhc_dev))
    {
      SwitchEhci2Xhci(*xhc_dev);
    }

    /**
     * 4.2 Host Controller Initialization
     */
    {
      auto err = xhc->Initialize();
      Log(kDebug, "xhc.Initialize: %s\n", err.Name());
    }
    /**
     * Set xHC to start executing commands or TDs
     */
    {
      auto err = xhc->Run();
      Log(kInfo, "xhc.Run: %s\n", err.Name());
    }

    __asm__("sti");

    usb::HIDMouseDriver::default_observer = MouseObserver;

    /**
     * 4.3 USB Device Initialization
     * the process of detecting and initializing a USB device attached to an xHC Root Hub port.
     *   - Reset a Root Hub Port
     *   - Device Slot Assignment
     *   - Device Slot Initialization
     *   - Address Assignment
     *   - Device Configuration
     *   - Setting Alternative Interface
     *   - Low-Speed/Full-Speed Device Support
     *   - Bandwidth Management
     * (https://www.intel.com/content/dam/www/public/us/en/documents/technical-specifications/extensible-host-controler-interface-usb-xhci.pdf)
     */
    /**
     * Loop through all USB ports,
     * do configure if not yet configured
     */
    for (int i = 1; i <= xhc->MaxPorts(); ++i)
    {
      auto port = xhc->PortAt(i);
      Log(kDebug, "Port %d: IsConnected=%d\n", i, port.IsConnected());

      if (port.IsConnected())
      {
        /**
         * TODO seems not conifg
         * - Reset If not connected (?)
         *
         * Argument-dependent lookup (ADL), usb::xhci::ConfigurePort
         */
        if (auto err = usb::xhci::ConfigurePort(*xhc, port))
        {
          Log(kError, "failed to configure port: %s at %s:%d\n", err.Name(), err.File(), err.Line());
          continue;
        }
      }
    }
  } // if (xhc_dev)

  while (1)
    __asm__("hlt");
  return;
}

/**
 * Define `__cxa_pure_virtual` to resolve "error: undefined symbol: __cxa_pure_virtual"
 * The function is also defined in libarary `c++abi`, so either add the library during linking,
 * or write it
 */
extern "C" void __cxa_pure_virtual()
{
  while (1)
    __asm__("hlt");
}
