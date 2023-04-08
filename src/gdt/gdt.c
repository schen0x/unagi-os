// gdt/gdt.c
#include "gdt/gdt.h"
#include "memory/memory.h"
#include "config.h"
#include "kernel.h"
#include "util/kutil.h"

static struct GlobalDescriptorTableRegister32 gdtr;
static struct GlobalDescriptorTable32 gdts[2];

extern void gdt_load(struct InterruptDescriptorTableRegister32* gdtr_ptr);

void gdt_zero()
{
	char msg[] = "Divide by zero error\n";
	kprint(msg, kstrlen(msg), 4);
}

void gdt_set(int interrupt_number, void* address)
{
	/* Set an "Ring 3 Interrupt Gate" interrupt to the `interrupt_number` */
	struct InterruptDescriptorTable32* gdt_entry = &gdts[interrupt_number];
	gdt_entry->offset_1 = (uint16_t)((uint32_t)address & 0x0000ffff);
	gdt_entry->selector = GDT_KERNEL_CODE_SEGMENT_SELECTOR;
	gdt_entry->zero = 0x0;
	gdt_entry->type_attr = 0xEE; // Ring 3 Intterrupt Gate, 0b1_11_0_1110 (0xEE)
	// gdt_entry->type_attr = 0x8E; // Ring 3 Intterrupt Gate, 0b1_11_0_1110 (0xEE)
	gdt_entry->offset_2 = (uint16_t)(((uint32_t)address >> 16) & 0x0000ffff);
}

void gdt_init()
{
	kmemset(gdts, 0, sizeof(gdts) * OS_IDT_TOTAL_INTERRUPTS); // Set the all gdt entries to 0

	/* Initialize the IDTR */
	gdtr.limit = sizeof(gdts) - 1;
	gdtr.base = (uint32_t) gdts;

	gdt_set(0, gdt_zero);
	gdt_load(&gdtr);
}


