#include "idt/idt.h"
#include "memory/memory.h"
#include "config.h"
#include "util/kutil.h"
#include "pic/pic.h"
#include "drivers/keyboard.h"
#include "include/uapi/graphic.h"
#include "util/printf.h"

/*
 * intptr_t _int_handlers_default[256] ; where _int_default_handlers[i] points to a default asm function
 * returns stack_frame* ptr and int32_t interrupt_number and pass to C
 */
extern intptr_t _int_default_handlers[OS_IDT_TOTAL_INTERRUPTS];

static IDT_IDTR_32 idtr = {0}; // static or not, global variable, address loaded into memory (probably .data), whose relative address to the entry is known in linktime.
static IDT_GATE_DESCRIPTOR_32 idts[OS_IDT_TOTAL_INTERRUPTS] = {0};

void idt_init()
{
	/* The kmemset is not necessary though, because a global variable will be auto initialized to 0 */
	// kmemset(idts, 0, sizeof(idts)); // Set the all idt entries to 0
	// kmemset(&idtr, 0, sizeof(idtr)); // Set the all idtr entries to 0

	/* Initialize the IDTR */
	idtr.size = (uint16_t)(sizeof(idts) - 1);
	idtr.offset = (uint32_t) idts;
	/* Set all interrupts to use the default handler */
	for (int i = 0; i < OS_IDT_TOTAL_INTERRUPTS; i++)
	{
		set_gatedesc(&idts[i], _int_default_handlers[i], OS_GDT_KERNEL_CODE_SEGMENT_SELECTOR, OS_IDT_AR_INTGATE32);
	}

	/* Set int21 to use _int21h */
	set_gatedesc(&idts[0x21], (intptr_t)&_int21h, OS_GDT_KERNEL_CODE_SEGMENT_SELECTOR, OS_IDT_AR_INTGATE32);
	_idt_load(&idtr);
	// Remap PIC after idt setup.
	PIC_remap(0x20, 0x28);
}

/*
 * Set idt (Gate Descriptor)
 * Example: set_gatedesc(&idts[0x21], (intptr_t)&_int21h, OS_GDT_KERNEL_CODE_SEGMENT_SELECTOR, OS_IDT_AR_INTGATE32);
 * selector: a valid code selector in gdt
 * offset: address to the routine, starting from the selector
 * access_right: e.g. AR_INTGATE32 0x8e
 */
void set_gatedesc(IDT_GATE_DESCRIPTOR_32* gd, intptr_t offset, uint16_t selector, uint8_t access_right)
{
	gd->offset_1 = (uint16_t)(offset & 0xffff);
	gd->selector = selector;
	gd->zero = 0x0;
	gd->type_attr = access_right;
	gd->offset_2 = (uint16_t)((offset >> 16) & 0xffff);
}

static void idt99()
{
	char msg[] = "idt 99";
	kfprint(msg, 4);
}

/*
 * The default handler for interrupts.
 * Can be called by asm functions on interruptions,
 * receiving the interrupt_number and pointer to the stack frame.
 */
void idt_int_default_handler(uint32_t interrupt_number, uintptr_t frame)
{
	(void)frame;

//	if(interrupt_number == 0x21)
//	{
//		_int21h();
//	}
	if(interrupt_number >= 0x20 && interrupt_number < 0x30)
	{
		PIC_sendEOI((uint8_t)((interrupt_number & 0xff) - 0x20)); // for 0x20-0x27, report to PIC0, 0:7; otherwise to PIC1, 8:15;
		return;
	}
	if(interrupt_number == 99)
	{
		idt99();
	}
}



void idt_zero()
{
	char msg[] = "Divide by zero error\n";
	kfprint(msg, 4);
}

/*
 * Keyboard interrupt handler
 * scancode: uint8_t _scancode + \0
 */
void int21h_handler(uint16_t scancode)
{
	atakbd_interrupt(scancode);
	PIC_sendEOI(1); // 21h, IRQ1
}

