#include "idt/idt.h"
#include "memory/memory.h"
#include "config.h"
#include "kernel.h"
#include "util/kutil.h"

/* global variable should be tudo initialized with 0 */
static struct InterruptDescriptorTableRegister32 idtr; // static or not, global variable, address loaded into memory, known in linktime.
static struct InterruptDescriptorTable32 idts[OS_IDT_TOTAL_INTERRUPTS];

extern void idt_load(struct InterruptDescriptorTableRegister32* idtr_ptr);
extern void _entry_inthandler_1();

void idt_zero()
{
	char msg[] = "Divide by zero error\n";
	kprint(msg, kstrlen(msg), 4);
}

void _inthandler_1()
{
	char msg[] = "Int 1\n";
	kprint(msg, kstrlen(msg), 4);
}

void idt_set(int interrupt_number, void* address)
{
	// char msg[2*sizeof(address)];
	// hex_to_ascii(address, msg, sizeof(address));
	// kprint(msg, sizeof(msg), 4);
	/* Set an "Ring 3 Interrupt Gate" interrupt to the `interrupt_number` */
	struct InterruptDescriptorTable32* idt_entry = &idts[interrupt_number];
	idt_entry->offset_1 = (uint16_t)((uint32_t)address & 0x0000ffff);
	idt_entry->selector = GDT_KERNEL_CODE_SEGMENT_SELECTOR;
	idt_entry->zero = 0x0;
	// idt_entry->type_attr = 0xEE; // Ring 3 Intterrupt Gate, 0b1_11_0_1110 (0xEE)
	idt_entry->type_attr = 0x8E; // Ring 0 Intterrupt Gate, 0b1_11_0_1110 (0xEE)
	idt_entry->offset_2 = (uint16_t)(((uint32_t)address >> 16) & 0x0000ffff);
}

void idt_init()
{
	/* The kmemset is not necessary though, because a global variable will be auto initialized to 0 */
	kmemset(idts, 0, sizeof(idts)); // Set the all idt entries to 0
	kmemset(&idtr, 0, sizeof(idtr)); // Set the all idtr entries to 0

	/* Initialize the IDTR */
	idtr.limit = (uint16_t) (sizeof(idts) - 1);
	idtr.base = (uint32_t) idts;

	idt_set(0, idt_zero);
	idt_set(1, _entry_inthandler_1);
	idt_load(&idtr);
}

