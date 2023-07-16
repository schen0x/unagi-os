/**
 * @file x86_descriptor.hpp
 */

#pragma once

/**
 * system segment & gate descriptor types
 * TODO ? messy?
 */
enum class DescriptorType
{
  kUpper8Bytes = 0,
  kLDT = 2,
  kTSSAvailable = 9,
  kTSSBusy = 11,
  /* (Intel) Gate Descriptor: 0xc (?) bit Call Gate (jump between different privilege levels) */
  kCallGate = 12,
  /* Gate Descriptor: 0xE; 32/64-bit Interrupt Gate */
  kInterruptGate = 14,
  /* Gate Descriptor: 0xF; 32/64-bit Trap Gate */
  kTrapGate = 15,

  // code & data segment types
  kReadWrite = 2,
  kExecuteRead = 10,
};
