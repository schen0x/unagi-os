#pragma once

#include <stdint.h>

extern "C"
{
  void __attribute__((sysv_abi)) IoOut32(uint16_t addr, uint32_t data);
  uint32_t __attribute__((sysv_abi)) IoIn32(uint16_t addr);
  uint16_t __attribute__((sysv_abi)) GetCS(void);
  void __attribute__((sysv_abi)) LoadIDT(uint16_t limit, uint64_t offset);
  void __attribute__((sysv_abi)) LoadGDT(uint16_t limit, uint64_t offset);
  void __attribute__((sysv_abi)) SetCSSS(uint16_t cs, uint16_t ss);
  /**
   * Set DS, ES, FS, GS to value
   */
  void __attribute__((sysv_abi)) SetDSAll(uint16_t value);
  void __attribute__((sysv_abi)) SetCR3(uint64_t value);
}
