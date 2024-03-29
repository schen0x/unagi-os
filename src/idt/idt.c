#include "config.h"
#include "drivers/graphic/videomode.h"
#include "drivers/keyboard.h"
#include "drivers/ps2kbc.h"
#include "idt/idt.h"
#include "include/uapi/graphic.h"
#include "include/uapi/input-mouse-event.h"
#include "io/io.h"
#include "memory/memory.h"
#include "memory/memory.h"
#include "pic/pic.h"
#include "pic/timer.h"
#include "status.h"
#include "util/kutil.h"
#include "util/printf.h"
#include "kernel.h"
#include "kernel/mprocessfifo.h"

/*
 * intptr_t _int_handlers_default[256] ; where _int_default_handlers[i] points to a default asm function
 * returns stack_frame* ptr and int32_t interrupt_number and pass to C
 */
extern intptr_t _int_default_handlers[OS_IDT_TOTAL_INTERRUPTS];

static IDT_IDTR_32 idtr = {0}; // static or not, global variable, address loaded into memory (probably .data), whose relative address to the entry is known in linktime.
static IDT_GATE_DESCRIPTOR_32 idts[OS_IDT_TOTAL_INTERRUPTS] = {0};
MPFIFO32 keymousefifo = {0};
int32_t _keymousefifobuf[512] = {0};

MPFIFO32* get_keymousefifo()
{
	return &keymousefifo;
}

void idt_init()
{
	mpfifo32_init(&keymousefifo, _keymousefifobuf, (sizeof(_keymousefifobuf) / sizeof(_keymousefifobuf[0])), NULL);

	/* Initialize the IDTR */
	idtr.size = (uint16_t)(sizeof(idts) - 1);
	idtr.offset = (uint32_t) idts;
	/* Set all interrupts to use the default handler */
	for (int i = 0; i < OS_IDT_TOTAL_INTERRUPTS; i++)
	{
		set_gatedesc(&idts[i], _int_default_handlers[i], OS_GDT_KERNEL_CODE_SEGMENT_SELECTOR, OS_IDT_AR_INTGATE32);
	}
	// /* Set int21 to use _int21h */
	// set_gatedesc(&idts[0x21], (intptr_t)&_int21h, OS_GDT_KERNEL_CODE_SEGMENT_SELECTOR, OS_IDT_AR_INTGATE32);

	_idt_load(&idtr);

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
 * The default handler (dispatcher?) for interrupts.
 * Can be called by asm functions on interruptions,
 * receiving the interrupt_number and pointer to the stack frame.
 */
void idt_int_default_handler(uint32_t interrupt_number, uintptr_t frame)
{
	(void)frame;

	/* int3 handler */
	if(interrupt_number == (0x3))
	{
		int3h();
		return;
	}

	if(interrupt_number == (0x20))
	{
		int20h();
		return;
	}
	if(interrupt_number == (0x2c))
	{
		int2ch();
		return;
	}

	if(interrupt_number == 0x21)
	{
		int21h();
		return;
	}
	if(interrupt_number >= 0x20 && interrupt_number < 0x30)
	{
		/* Spurious IRQs Handling (makeshift, no EOI) */
		if (interrupt_number == 0x20 + 7 || interrupt_number == 0x28 + 7)
			return;
		PIC_sendEOI((uint8_t)((interrupt_number & 0xff) - 0x20)); // for 0x20-0x27, report to PIC0, 0:7; otherwise to PIC1, 8:15;
		return;
	}
	if(interrupt_number == 99)
	{
		idt99();
	}
}

/*
 * `int3` handler
 * Allow inline breakpoint and resume execution
 */

void int3h(void)
{
	return;
}


/*
 * PS2KBC MOUSE Interrupt
 */
void int2ch(void)
{
	uint8_t volatile data = _io_in8(PS2KBC_PORT_DATA_RW);
	PIC_sendEOI(12); // 2ch, IRQ12
	mpfifo32_enqueue(&keymousefifo, data + DEV_FIFO_MOUSE_START);
	return;
}


void idt_zero()
{
	char msg[] = "Divide by zero error\n";
	kfprint(msg, 4);
}

/* ?Race condition/Memory corruption? */
void int21h(void)
{
	uint8_t volatile data = _io_in8(PS2KBC_PORT_DATA_RW);
	PIC_sendEOI(1); // 21h, IRQ1
	mpfifo32_enqueue(&keymousefifo, data+DEV_FIFO_KBD_START);
	return;
}


/**
 * Timer
 * NOTE: EOI should be sent before a complex handler, because if taskswitch happens
 * within the handler, the EOI may never be sent depends on the code flow (if the
 * code never returns, and will sleep until interrupt), causing a deadlock
 */
void int20h()
{
	/* ? TODO what(when) is the correct time to send the EOI? */
	PIC_sendEOI(0); // 20h, IRQ0
	timer_int_handler();
	return;
}
