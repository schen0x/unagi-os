/**
 * @file interrupt.cpp
 *
 * Interrupt Descriptor Table (IDT)
 */

#include "interrupt.hpp"

// #@@range_begin(idt_array)
std::array<InterruptDescriptor, 256> idt;
// #@@range_end(idt_array)

// #@@range_begin(set_idt_entry)
void SetIDTEntry(InterruptDescriptor &desc, InterruptDescriptorAttribute attr, uint64_t offset,
                 uint16_t segment_selector)
{
  desc.attr = attr;
  desc.offset_low = offset & 0xffffu;
  desc.offset_middle = (offset >> 16) & 0xffffu;
  desc.offset_high = offset >> 32;
  desc.segment_selector = segment_selector;
}
// #@@range_end(set_idt_entry)

// #@@range_begin(notify_eoi)
void __attribute__((no_caller_saved_registers)) NotifyEndOfInterrupt()
{
  /**
   * TODO x86, PCI, or APIC ?
   * End Of Interrupt (EOI) Reg. 0xFEE000B0
   * Write 0 then CPU will be informed EOI
   */
  volatile auto end_of_interrupt = reinterpret_cast<uint32_t *>(0xfee000b0);
  *end_of_interrupt = 0;
}
// #@@range_end(notify_eoi)
