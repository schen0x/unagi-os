#ifndef GDT_GDT_H_
#define GDT_GDT_H_

#include <stdint.h>
void idt_zero();
void idt_init();

/**
 * Global Descriptor Table Entry, 8 bytes
 */
typedef struct GDT32
{
	/* limit 0:15 */
	uint16_t limit_low;
	/* base 0:15 */
	uint16_t base_low;
	/* base 16:23 */
	uint8_t base_mid;
	/**
	 * From 0:7 is:
	 * 	Access bit
	 * 	RW bit
	 * 	DC bit
	 * 	Executable bit
	 * 	System segment bit
	 * 	DPL 2bits
	 * 	Present bit
	 *
	 * For a non-system segment (code/data segment)
	 * 	0x9a: Kernel Mode Code Segment (Flags 0xc)
	 * 	0x92: Kernel Mode Data Segment (Flags 0xc)
	 * 	0xfa: User Mode Code Segment (Flags 0xc)
	 * 	0xf2: User Mode Data Segment (Flags 0xc)
	 * 	0x89: Task State Segment (Flag 0x0)
	 *
	 * For a system segment:
	 * Types available in 32-bit protected mode:
	 *	0x1: 16-bit TSS (Available)
	 *	0x2: LDT
	 *	0x3: 16-bit TSS (Busy)
	 *	0x9: 32-bit TSS (Available)
	 *	0xB: 32-bit TSS (Busy)
	 */
	uint8_t access_byte;
	/* limit 16:19 */
	uint32_t limit_high : 4;
	/**
	 * Normally(?) 0xc (0b1100 for 4KB & 32bit protected mode segment)
	 *
	 * From 0:3 is
	 * 	0
	 * 	L: 1 for Long-mode
	 * 	Size: Segment Selector size; 1 for 32-bytes protected mode; TODO is V8086 relevant?
	 * 	Granularity: 0 for byte, 1 for 4KB
	 */
	uint32_t flags : 4;
	/* base 24:31 */
	uint8_t base_high;
} __attribute__((packed)) GDT32;

/**
 * Global Descriptor Table Entry Register
 */
typedef struct GDTR32
{
	uint16_t limit; // total gdt entry counts minus 1
	uint32_t base; // uintptr GDT_START
} __attribute__((packed)) GDTR32;

void gdt_set(int interrupt_number, void* address);
void gdt_init();
extern void _gdt_load(GDTR32 *gdtr);
extern void _gdt_load_task_register(uint16_t tss_segment_selector);

#endif

