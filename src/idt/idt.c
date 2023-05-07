#include "idt/idt.h"
#include "memory/memory.h"
#include "config.h"
#include "include/uapi/graphic.h"
#include "include/uapi/input-mouse-event.h"
#include "io/io.h"
#include "drivers/keyboard.h"
#include "drivers/ps2mouse.h"
#include "drivers/ps2kbc.h"
#include "drivers/graphic/videomode.h"
#include "util/kutil.h"
#include "util/printf.h"
#include "memory/memory.h"
#include "pic/pic.h"
#include "status.h"
#include "pic/timer.h"

/*
 * intptr_t _int_handlers_default[256] ; where _int_default_handlers[i] points to a default asm function
 * returns stack_frame* ptr and int32_t interrupt_number and pass to C
 */
extern intptr_t _int_default_handlers[OS_IDT_TOTAL_INTERRUPTS];

static IDT_IDTR_32 idtr = {0}; // static or not, global variable, address loaded into memory (probably .data), whose relative address to the entry is known in linktime.
static IDT_GATE_DESCRIPTOR_32 idts[OS_IDT_TOTAL_INTERRUPTS] = {0};
FIFO32 keymousefifo = {0};
int32_t _keymousefifobuf[4096] = {0};

/* 0 before 0xfa; 1 afterwards */
//static int32_t mouse_phase = 0;
//FIFO32 mouse_one_move_buf = {0}; // 3 bytes
//     			//
//const int32_t MOUSE_ONE_MOVE_CMD_SIZE = 3;
//uint8_t _mouse_one_move_buf[3] = {0}; // 3 bytes[]
				      //
MOUSE_DATA_BUNDLE mouse_one_move = {0};

static void chips_init()
{
	/* TODO refactoring, find somewhere else for the buffer */
	// uint8_t _keybuf[32] = {0};
	// uint8_t *_keybuf = (uint8_t*) kzalloc(32); // of course if memory it not yet initialized, this is not possible.
	//! So the problem is that the idt_init() will return, while eventloop running
	//! So if _keybuf uses a stack address, it causes panic
	//! uint8_t _keybuf[32] = {0};
	fifo32_init(&keymousefifo, _keymousefifobuf, sizeof(_keymousefifobuf));
	// fifo8_init(&mouse_one_move_buf, _mouse_one_move_buf, sizeof(_mouse_one_move_buf));

	ps2kbc_KBC_init();
	ps2kbc_MOUSE_init();

	pit_init();
}

void idt_init()
{
	chips_init();
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
		// int21h();
		__int21h_buffed();
		return;
	}
	if(interrupt_number >= 0x20 && interrupt_number < 0x30)
	{
		//char i[10] = {0};
		//sprintf(i, "%x", interrupt_number);
		//kfprint(i, 4);
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
	fifo32_enqueue(&keymousefifo, data + DEV_FIFO_MOUSE_START);
	return;
}


/*
 * MOUSE data handler
 * scancode: uint8_t _scancode
 */
void int2ch_handler(uint8_t scancode)
{
	ps2mouse_decode(scancode, &mouse_one_move);
	graphic_move_mouse(&mouse_one_move);
}


void idt_zero()
{
	char msg[] = "Divide by zero error\n";
	kfprint(msg, 4);
}

/* ?Race condition/Memory corruption? */
void __int21h_buffed()
{
	uint8_t volatile data = _io_in8(PS2KBC_PORT_DATA_RW);
	// char buf[20]={0};
	// sprintf(buf, "_%02x_", (uint8_t)data);
	// kfprint(buf, 4);
	PIC_sendEOI(1); // 21h, IRQ1
	fifo32_enqueue(&keymousefifo, data+DEV_FIFO_KBD_START);
	return; // ? from sequential to cpu polling buffer. int21h_handler(data); in another thread?
}


/**
 * Timer
 */
void int20h()
{
	timer_int_handler();
	PIC_sendEOI(0); // 20h, IRQ0
	return;
}
