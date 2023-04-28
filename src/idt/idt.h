#ifndef IDT_IDT_H_
#define IDT_IDT_H_

#include <stdint.h>
#include "util/fifo.h"
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

typedef struct MOUSE_DATA_BUNDLE
{
	uint8_t buf[3], phase;
	int32_t x, y, btn;
} MOUSE_DATA_BUNDLE;

extern void _idt_load(IDT_IDTR_32* idtr_ptr);
void set_gatedesc(IDT_GATE_DESCRIPTOR_32* gd, intptr_t offset, uint16_t selector, uint8_t access_right);
void idt_init();
void idt_int_default_handler(uint32_t interrupt_number, uintptr_t frame);
void __int21h_buffed();
void int21h_handler(uint8_t scancode);
void int2ch(void);
void int2ch_handler(uint8_t scancode);
int32_t ps2mouse_parse_three_bytes(MOUSE_DATA_BUNDLE *m);
static int32_t parse_twos_compliment(int32_t signed_bit, uint8_t tail);
extern FIFO8 keybuf;
extern FIFO8 mousebuf;

void idt_zero();
void int21h();


#endif
