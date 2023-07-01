/**
 * @file interrupt.cpp
 *
 * Interrupt Descriptor Table (IDT)
 */

#include "interrupt.hpp"

std::array<InterruptDescriptor, 256> idt;

void SetIDTEntry(InterruptDescriptor &desc, InterruptDescriptorAttribute attr, uint64_t offset,
                 uint16_t segment_selector)
{
  desc.attr = attr;
  desc.offset_low = offset & 0xffffu;
  desc.offset_middle = (offset >> 16) & 0xffffu;
  desc.offset_high = offset >> 32;
  desc.segment_selector = segment_selector;
}

/**
 * Inform a CPU core of the EOI
 * TODO is 8259 PIC disabled?
 */
void __attribute__((no_caller_saved_registers)) NotifyEndOfInterrupt()
{
  /**
   * APIC (More detail on OSDEV); in particular, should be "Local APIC" here
   * The address is defined in the "Intel SDM";
   * Table 11-1. Local APIC Register Address Map (1-4, p3389)
   *   - 0xFEE000B0: Local APIC "End Of Interrupt (EOI)" Register
   * Write 0 then the CPU core will be informed EOI
   */
  volatile auto end_of_interrupt = reinterpret_cast<uint32_t *>(0xfee000b0);
  *end_of_interrupt = 0;
}
