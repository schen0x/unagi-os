/**
 * @file pci.hpp
 *
 * Interact with PCI Bus
 */

#pragma once

#include <array>
#include <cstdint>

#include "error.hpp"

namespace pci
{
/** @brief IO port for the CONFIG_ADDRESS register */
const uint16_t kConfigAddress = 0x0cf8;
/** @brief IO port for the CONFIG_DATA register */
const uint16_t kConfigData = 0x0cfc;

// #@@range_begin(class_code)
/** @brief PCI デバイスのクラスコード */
struct ClassCode
{
  uint8_t base, sub, interface;

  /** @brief ベースクラスが等しい場合に真を返す */
  bool Match(uint8_t b)
  {
    return b == base;
  }
  /** @brief ベースクラスとサブクラスが等しい場合に真を返す */
  bool Match(uint8_t b, uint8_t s)
  {
    return Match(b) && s == sub;
  }
  /** @brief ベース，サブ，インターフェースが等しい場合に真を返す */
  bool Match(uint8_t b, uint8_t s, uint8_t i)
  {
    return Match(b, s) && i == interface;
  }
};

/** @brief PCI デバイスを操作するための基礎データを格納する
 *
 * バス番号，デバイス番号，ファンクション番号はデバイスを特定するのに必須．
 * その他の情報は単に利便性のために加えてある．
 * */
struct Device
{
  uint8_t bus, device, function, header_type;
  ClassCode class_code;
};
// #@@range_end(class_code)

/** @brief CONFIG_ADDRESS に指定された整数を書き込む */
void WriteAddress(uint32_t address);
/** @brief CONFIG_DATA に指定された整数を書き込む */
void WriteData(uint32_t value);
/** @brief CONFIG_DATA から 32 ビット整数を読み込む */
uint32_t ReadData();

/** @brief ベンダ ID レジスタを読み取る（全ヘッダタイプ共通） */
uint16_t ReadVendorId(uint8_t bus, uint8_t device, uint8_t function);
/** @brief デバイス ID レジスタを読み取る（全ヘッダタイプ共通） */
uint16_t ReadDeviceId(uint8_t bus, uint8_t device, uint8_t function);
/** @brief ヘッダタイプレジスタを読み取る（全ヘッダタイプ共通） */
uint8_t ReadHeaderType(uint8_t bus, uint8_t device, uint8_t function);
/** @brief クラスコードレジスタを読み取る（全ヘッダタイプ共通） */
ClassCode ReadClassCode(uint8_t bus, uint8_t device, uint8_t function);

inline uint16_t ReadVendorId(const Device &dev)
{
  return ReadVendorId(dev.bus, dev.device, dev.function);
}

/** @brief 指定された PCI デバイスの 32 ビットレジスタを読み取る */
uint32_t ReadConfReg(const Device &dev, uint8_t reg_addr);

void WriteConfReg(const Device &dev, uint8_t reg_addr, uint32_t value);

uint32_t ReadBusNumbers(uint8_t bus, uint8_t device, uint8_t function);

bool IsSingleFunctionDevice(uint8_t header_type);

/** @brief All devices found via ScanAllBus() */
inline std::array<Device, 32> devices;
/** @brief Total effective devices found; devices[num_device] is empty */
inline int num_device;
/** @brief PCI デバイスをすべて探索し devices に格納する
 *
 * バス 0 から再帰的に PCI デバイスを探索し，devices の先頭から詰めて書き込む．
 * 発見したデバイスの数を num_devices に設定する．
 */
Error ScanAllBus();

/**
 * Base Address Registers (BARs) address
 * For HeaderType == 0x0, 5BARs, offset 0x10
 * For HeaderType == 0x1, 2BARs, offset 0x10
 */
constexpr uint8_t CalcBarAddress(unsigned int bar_index)
{
  return 0x10 + 4 * bar_index;
}

WithError<uint64_t> ReadBar(Device &device, unsigned int bar_index);
} // namespace pci
