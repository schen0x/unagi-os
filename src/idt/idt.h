#ifndef IDT_IDT_H_
#define IDT_IDT_H_

#include <stdint.h>
typedef struct IDT_GATE_DESCRIPTOR_32
{
	/*
	 * Offset: A 32-bit value, split in two parts. It represents the
	 * address of the entry point of the Interrupt Service Routine.
	 * offset_1: Offset bits 0:15
	 */
	uint16_t offset_1;
	/* Selector: A Segment Selector with multiple fields which must point to a valid code segment in your GDT. */
	uint16_t selector;
	uint8_t zero; // unused byte
	/*
	 * Gate Type
	 * 32-bit Intterrupt Gate kernel access == 0x8E
	 */
	uint8_t type_attr;
	/*
	 * Offset: A 32-bit value, split in two parts. It represents the
	 * address of the entry point of the Interrupt Service Routine.
	 * offset_2: Offset bits 16:31
	 */
	uint16_t offset_2;
} __attribute__((packed)) IDT_GATE_DESCRIPTOR_32;

/* IDT Descriptor (IDTR) */
typedef struct IDT_IDTR_32
{
	/* One less than the size of the IDT in bytes */
	uint16_t size;
	/* The linear address of the Interrupt Descriptor Table (not the physical address, paging applies) */
	uint32_t offset;
} __attribute__((packed)) IDT_IDTR_32;

typedef struct KEYBUF {
	uint8_t data[32];
	int32_t next_r, next_w, len;
} KEYBUF;

extern void _idt_load(IDT_IDTR_32* idtr_ptr);
void set_gatedesc(IDT_GATE_DESCRIPTOR_32* gd, intptr_t offset, uint16_t selector, uint8_t access_right);
void idt_init();
void idt_int_default_handler(uint32_t interrupt_number, uintptr_t frame);
void int21h_handler(uint16_t keyPressed);
void int21h();
void __int21h_buffed();
extern KEYBUF keybuf;

void idt_zero();


#endif
