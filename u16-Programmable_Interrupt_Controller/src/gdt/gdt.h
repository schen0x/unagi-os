#ifndef IDT_IDT_H_
#define IDT_IDT_H_

#include <stdint.h>
void idt_zero();
void idt_init();

struct InterruptDescriptorTable32
{
	uint16_t offset_1; // Offset bits 0-15
	uint16_t selector; // Selector in the GDT
	uint8_t zero; // unused byte
	uint8_t type_attr; // Descriptor type and attributes, 0b1 + DPL (2bits) + 0 + Gate Type (3bit); Ring 3 Intterrupt Gate is 0b1_11_0_1110 (0xEE)
	uint16_t offset_2; // Offset bits 16-31
} __attribute__((packed));

struct InterruptDescriptorTableRegister32
{
	uint16_t limit; // total idt counts minus 1
	uint32_t base; // the address, paging applies, where the IDTR starts
} __attribute__((packed));

void idt_set(int interrupt_number, void* address);
void idt_init();

#endif
