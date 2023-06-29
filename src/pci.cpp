/**
 * @file pci.cpp
 *
 * Interact with PCI Bus
 */

#include "pci.hpp"

#include "asmfunc.h"
#include "logger.hpp"
#include "sys/_stdint.h"

namespace
{
using namespace pci;

/** @brief Make the uint32_t CONFIG_ADDRESS */
uint32_t MakeAddress(uint8_t bus, uint8_t device, uint8_t function, uint8_t reg_addr)
{
  auto shl = [](uint32_t x, unsigned int bits) { return x << bits; };

  /**
   * reg_addr & 0xfc
   *   - Register Offset has to point to consecutive DWORDs, i.e., bits 1:0 are
   *   always 0b00 (they are still part of the Register Offset).
   */
  return shl(1, 31) // enable bit
         | shl(bus, 16) | shl(device, 11) | shl(function, 8) | (reg_addr & 0xfcu);
}

/**
 * @brief Store the device found in a global variable
 * - devices[num_device] <- device found
 * - num_device++
 */
Error AddDevice(const Device &device)
{
  if (num_device == devices.size())
  {
    return MAKE_ERROR(Error::kFull);
  }

  devices[num_device] = device;
  ++num_device;
  return MAKE_ERROR(Error::kSuccess);
}

Error ScanBus(uint8_t bus);

/** @brief
 * - Add the function to devices
 * - If the target is a PCI-to-PCI bridge (HeaderType == 0x1) (QEMU seems to not emulate this by default),
 *   find out the secondary_bus (device header offset 0x18)
 *   then explore the secondary_bus first, before exploring other functions (DFS)
 */
Error ScanFunction(uint8_t bus, uint8_t device, uint8_t function)
{
  auto class_code = ReadClassCode(bus, device, function);
  auto header_type = ReadHeaderType(bus, device, function);
  Device dev{bus, device, function, header_type, class_code};
  if (auto err = AddDevice(dev))
  {
    return err;
  }

  /* PCI class code, 0x06 bridge, 0x04 PCI-to-PCI bridge */
  if (class_code.Match(0x06u, 0x04u))
  {
    auto bus_numbers = ReadBusNumbers(bus, device, function);
    uint8_t secondary_bus = (bus_numbers >> 8) & 0xffu;
    return ScanBus(secondary_bus);
  }

  return MAKE_ERROR(Error::kSuccess);
}

/** @brief Scan functions of a device (on a bus)
 * - ScanFunction 0 (it must exists)
 * - Read HeaderType, if multi-function device, continue ScanFunction
 */
Error ScanDevice(uint8_t bus, uint8_t device)
{
  if (auto err = ScanFunction(bus, device, 0))
  {
    return err;
  }
  if (IsSingleFunctionDevice(ReadHeaderType(bus, device, 0)))
  {
    return MAKE_ERROR(Error::kSuccess);
  }

  for (uint8_t function = 1; function < 8; ++function)
  {
    if (ReadVendorId(bus, device, function) == 0xffffu)
    {
      continue;
    }
    if (auto err = ScanFunction(bus, device, function))
    {
      return err;
    }
  }
  return MAKE_ERROR(Error::kSuccess);
}

/** @brief Scan a bus
 * - Read VenderId of a device
 * - If the VenderId is valid, do ScanDevice
 */
Error ScanBus(uint8_t bus)
{
  /* device is bit 15:11, [0, 31] */
  for (uint8_t device = 0; device < 32; ++device)
  {
    /* 0xffff means non-existent device, per PCI spec */
    if (ReadVendorId(bus, device, 0) == 0xffffu)
    {
      continue;
    }
    if (auto err = ScanDevice(bus, device))
    {
      return err;
    }
  }
  return MAKE_ERROR(Error::kSuccess);
}
/** @brief 指定された MSI ケーパビリティ構造を読み取る
 *
 * @param dev  MSI ケーパビリティを読み込む PCI デバイス
 * @param cap_addr  MSI ケーパビリティレジスタのコンフィグレーション空間アドレス
 */
MSICapability ReadMSICapability(const Device &dev, uint8_t cap_addr)
{
  MSICapability msi_cap{};

  msi_cap.header.data = ReadConfReg(dev, cap_addr);
  msi_cap.msg_addr = ReadConfReg(dev, cap_addr + 4);

  uint8_t msg_data_addr = cap_addr + 8;
  if (msi_cap.header.bits.addr_64_capable)
  {
    msi_cap.msg_upper_addr = ReadConfReg(dev, cap_addr + 8);
    msg_data_addr = cap_addr + 12;
  }

  msi_cap.msg_data = ReadConfReg(dev, msg_data_addr);

  if (msi_cap.header.bits.per_vector_mask_capable)
  {
    msi_cap.mask_bits = ReadConfReg(dev, msg_data_addr + 4);
    msi_cap.pending_bits = ReadConfReg(dev, msg_data_addr + 8);
  }

  return msi_cap;
}

/** @brief 指定された MSI ケーパビリティ構造に書き込む
 *
 * @param dev  MSI ケーパビリティを読み込む PCI デバイス
 * @param cap_addr  MSI ケーパビリティレジスタのコンフィグレーション空間アドレス
 * @param msi_cap  書き込む値
 */
void WriteMSICapability(const Device &dev, uint8_t cap_addr, const MSICapability &msi_cap)
{
  WriteConfReg(dev, cap_addr, msi_cap.header.data);
  WriteConfReg(dev, cap_addr + 4, msi_cap.msg_addr);

  uint8_t msg_data_addr = cap_addr + 8;
  if (msi_cap.header.bits.addr_64_capable)
  {
    WriteConfReg(dev, cap_addr + 8, msi_cap.msg_upper_addr);
    msg_data_addr = cap_addr + 12;
  }

  WriteConfReg(dev, msg_data_addr, msi_cap.msg_data);

  if (msi_cap.header.bits.per_vector_mask_capable)
  {
    WriteConfReg(dev, msg_data_addr + 4, msi_cap.mask_bits);
    WriteConfReg(dev, msg_data_addr + 8, msi_cap.pending_bits);
  }
}

/** @brief 指定された MSI レジスタを設定する */
Error ConfigureMSIRegister(const Device &dev, uint8_t cap_addr, uint32_t msg_addr, uint32_t msg_data,
                           unsigned int num_vector_exponent)
{
  auto msi_cap = ReadMSICapability(dev, cap_addr);

  if (msi_cap.header.bits.multi_msg_capable <= num_vector_exponent)
  {
    msi_cap.header.bits.multi_msg_enable = msi_cap.header.bits.multi_msg_capable;
  }
  else
  {
    msi_cap.header.bits.multi_msg_enable = num_vector_exponent;
  }

  msi_cap.header.bits.msi_enable = 1;
  msi_cap.msg_addr = msg_addr;
  msi_cap.msg_data = msg_data;

  WriteMSICapability(dev, cap_addr, msi_cap);
  return MAKE_ERROR(Error::kSuccess);
}

/** @brief 指定された MSI レジスタを設定する */
Error ConfigureMSIXRegister(const Device &dev, uint8_t cap_addr, uint32_t msg_addr, uint32_t msg_data,
                            unsigned int num_vector_exponent)
{
  return MAKE_ERROR(Error::kNotImplemented);
}
} // namespace

/**
 * Interact with the pci 256-byte Configuration Space registers
 *
 * All PCI compliant devices must support the Vendor ID, Device ID, Command and
 * Status, Revision ID, Class Code and Header Type fields. Implementation of
 * the other registers is optional, depending upon the devices functionality.
 * ref: (https://wiki.osdev.org/PCI)
 */
namespace pci
{
void WriteAddress(uint32_t address)
{
  IoOut32(kConfigAddress, address);
}

void WriteData(uint32_t value)
{
  IoOut32(kConfigData, value);
}

uint32_t ReadData()
{
  return IoIn32(kConfigData);
}

uint16_t ReadVendorId(uint8_t bus, uint8_t device, uint8_t function)
{
  /* VenderId, offset 0, bit 0:15 */
  WriteAddress(MakeAddress(bus, device, function, 0x00));
  return ReadData() & 0xffffu;
}

uint16_t ReadDeviceId(uint8_t bus, uint8_t device, uint8_t function)
{
  /* DeviceId, offset 0, bit 16:31 */
  WriteAddress(MakeAddress(bus, device, function, 0x00));
  return ReadData() >> 16;
}

/**
 * Header Type:
 * - 0x0 a general device
 * - 0x1 a PCI-to-PCI bridge
 * - 0x2 a PCI-to-CardBus bridge
 */
uint8_t ReadHeaderType(uint8_t bus, uint8_t device, uint8_t function)
{
  /* HeaderType, offset 0xc, bit 16:23 */
  WriteAddress(MakeAddress(bus, device, function, 0x0c));
  return (ReadData() >> 16) & 0xffu;
}

/* (ClassCode(31:24) + Subclass(23:16) + Prog If(15:8) + Revision ID(7:0), offset 0xc */
ClassCode ReadClassCode(uint8_t bus, uint8_t device, uint8_t function)
{
  WriteAddress(MakeAddress(bus, device, function, 0x08));
  auto reg = ReadData();
  ClassCode cc;
  cc.base = (reg >> 24) & 0xffu;
  cc.sub = (reg >> 16) & 0xffu;
  cc.interface = (reg >> 8) & 0xffu;
  return cc;
}

/**
 * Only applicable when HeaderType == 0x1 (PCI-to-PCI bridge), read the:
 * - Secondary Latency Timer(31:24)
 *   Subordinate Bus Number(23:16)
 *   Secondary Bus Number(15:8)
 *   Primary Bus Number(7:0)
 */
uint32_t ReadBusNumbers(uint8_t bus, uint8_t device, uint8_t function)
{
  WriteAddress(MakeAddress(bus, device, function, 0x18));
  return ReadData();
}

/**
 * If bit 7 of the Header Type is set, the device has multiple functions;
 * Otherwise, it is a single function device
 */
bool IsSingleFunctionDevice(uint8_t header_type)
{
  return (header_type & 0x80u) == 0;
}

/**
 * Enem All Buses
 * Start from Bus 0, Device 0, Func 0; Usually Host Bridge Root Complex
 * - Assume Single function
 * - Do depth first search: If Device HeaderType indicate PCI-to-PCI bridge,
 *   Before proceeding to discover additional functions/devices on bus 0, it
 *   must proceed to search bus 1
 */
Error ScanAllBus()
{
  /* global variable, reset devices counter to 0 before rescan  */
  num_device = 0;

  auto header_type = ReadHeaderType(0, 0, 0);
  if (IsSingleFunctionDevice(header_type))
  {
    return ScanBus(0);
  }

  /* Otherwise, all functions have to be scanned */
  for (uint8_t function = 0; function < 8; ++function)
  {
    if (ReadVendorId(0, 0, function) == 0xffffu)
    {
      continue;
    }
    if (auto err = ScanBus(function))
    {
      return err;
    }
  }
  return MAKE_ERROR(Error::kSuccess);
}

uint32_t ReadConfReg(const Device &dev, uint8_t reg_addr)
{
  WriteAddress(MakeAddress(dev.bus, dev.device, dev.function, reg_addr));
  return ReadData();
}

void WriteConfReg(const Device &dev, uint8_t reg_addr, uint32_t value)
{
  WriteAddress(MakeAddress(dev.bus, dev.device, dev.function, reg_addr));
  WriteData(value);
}

/**
 * Base Address Registers
 * - 16-Byte Aligned Base Address 31:4
 * - Prefetchable 3
 * - Type 2:1
 * - Always 0 0
 */
WithError<uint64_t> ReadBar(Device &device, unsigned int bar_index)
{
  if (bar_index >= 6)
  {
    return {0, MAKE_ERROR(Error::kIndexOutOfRange)};
  }

  const uint8_t addr = CalcBarAddress(bar_index);
  const uint32_t bar = ReadConfReg(device, addr);

  /* 0b00 0: 32 bit address;
   * 0b10 0: 64 bit address;
   * 0b01 0: reserved for revision 3.0 of PCI Local Bus specification
   */
  if ((bar & 4u) == 0)
  {
    return {bar, MAKE_ERROR(Error::kSuccess)};
  }

  // 64 bit address, an address consumes 2 bar_indexes, so uplimit -= 1
  if (bar_index >= 5)
  {
    return {0, MAKE_ERROR(Error::kIndexOutOfRange)};
  }

  const uint32_t bar_upper = ReadConfReg(device, addr + 4);
  return {bar | (static_cast<uint64_t>(bar_upper) << 32), MAKE_ERROR(Error::kSuccess)};
}

CapabilityHeader ReadCapabilityHeader(const Device &dev, uint8_t addr)
{
  CapabilityHeader header;
  header.data = pci::ReadConfReg(dev, addr);
  return header;
}

Error ConfigureMSI(const Device &dev, uint32_t msg_addr, uint32_t msg_data, unsigned int num_vector_exponent)
{
  uint8_t cap_addr = ReadConfReg(dev, 0x34) & 0xffu;
  uint8_t msi_cap_addr = 0, msix_cap_addr = 0;
  while (cap_addr != 0)
  {
    auto header = ReadCapabilityHeader(dev, cap_addr);
    if (header.bits.cap_id == kCapabilityMSI)
    {
      msi_cap_addr = cap_addr;
    }
    else if (header.bits.cap_id == kCapabilityMSIX)
    {
      msix_cap_addr = cap_addr;
    }
    cap_addr = header.bits.next_ptr;
  }

  if (msi_cap_addr)
  {
    return ConfigureMSIRegister(dev, msi_cap_addr, msg_addr, msg_data, num_vector_exponent);
  }
  else if (msix_cap_addr)
  {
    return ConfigureMSIXRegister(dev, msix_cap_addr, msg_addr, msg_data, num_vector_exponent);
  }
  return MAKE_ERROR(Error::kNoPCIMSI);
}

/**
 * Intel 64 Software Developer's Manual Vol.3A
 *   - 11.11 MESSAGE SIGNALLED INTERRUPTS (1-4, p3418)
 *
 * @vector the interrupt number; a writable field in the Message Data Register
 */
Error ConfigureMSIFixedDestination(const Device &dev, uint8_t apic_id, MSITriggerMode trigger_mode,
                                   MSIDeliveryMode delivery_mode, uint8_t vector, unsigned int num_vector_exponent)
{
  uint32_t msg_addr = 0xfee00000u | (apic_id << 12);
  uint32_t msg_data = (static_cast<uint32_t>(delivery_mode) << 8) | vector;
  if (trigger_mode == MSITriggerMode::kLevel)
  {
    msg_data |= 0xc000;
  }
  return ConfigureMSI(dev, msg_addr, msg_data, num_vector_exponent);
}
} // namespace pci

void InitializePCI()
{
  if (auto err = pci::ScanAllBus())
  {
    Log(kError, "ScanAllBus: %s\n", err.Name());
    exit(1);
  }

  for (int i = 0; i < pci::num_device; ++i)
  {
    const auto &dev = pci::devices[i];
    auto vendor_id = pci::ReadVendorId(dev);
    auto class_code = pci::ReadClassCode(dev.bus, dev.device, dev.function);
    Log(kDebug, "%d.%d.%d: vend %04x, class %08x, head %02x\n", dev.bus, dev.device, dev.function, vendor_id,
        class_code, dev.header_type);
  }
} // namespace pci
