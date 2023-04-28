#include "idt/idt.h"
#include "memory/memory.h"
#include "config.h"
#include "util/kutil.h"
#include "pic/pic.h"
#include "drivers/keyboard.h"
#include "include/uapi/graphic.h"
#include "util/printf.h"
#include "io/io.h"
#include "drivers/ps2kbc.h"
#include "memory/memory.h"
#include "status.h"
#include "drivers/graphic/videomode.h"

/*
 * intptr_t _int_handlers_default[256] ; where _int_default_handlers[i] points to a default asm function
 * returns stack_frame* ptr and int32_t interrupt_number and pass to C
 */
extern intptr_t _int_default_handlers[OS_IDT_TOTAL_INTERRUPTS];

static IDT_IDTR_32 idtr = {0}; // static or not, global variable, address loaded into memory (probably .data), whose relative address to the entry is known in linktime.
static IDT_GATE_DESCRIPTOR_32 idts[OS_IDT_TOTAL_INTERRUPTS] = {0};
FIFO8 keybuf = {0};
FIFO8 mousebuf = {0};
uint8_t _keybuf[32] = {0};
uint8_t _mousebuf[128] = {0};

/* 0 before 0xfa; 1 afterwards */
//static int32_t mouse_phase = 0;
//FIFO8 mouse_one_move_buf = {0}; // 3 bytes
//     			//
//const int32_t MOUSE_ONE_MOVE_CMD_SIZE = 3;
//uint8_t _mouse_one_move_buf[3] = {0}; // 3 bytes[]
				      //
MOUSE_DATA_BUNDLE mouse_one_move = {0};

void idt_init()
{
	/* TODO refactoring, find somewhere else for the buffer */
	// uint8_t _keybuf[32] = {0};
	// uint8_t *_keybuf = (uint8_t*) kzalloc(32); // of course if memory it not yet initialized, this is not possible.
	//! So the problem is that the idt_init() will return, while eventloop running
	//! So if _keybuf uses a stack address, it causes panic
	//! uint8_t _keybuf[32] = {0};
	fifo8_init(&keybuf, _keybuf, sizeof(_keybuf));
	fifo8_init(&mousebuf, _mousebuf, sizeof(_mousebuf));
	// fifo8_init(&mouse_one_move_buf, _mouse_one_move_buf, sizeof(_mouse_one_move_buf));

	ps2kbc_KBC_init();
	ps2kbc_MOUSE_init();
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
 * PS2KBC MOUSE Interrupt
 */
void int2ch(void)
{
	uint8_t volatile data = _io_in8(PS2KBC_PORT_DATA_RW);
//	char i[10] = {0};
//	sprintf(i, "KBCDT:%2x ", data);
//	kfprint(i, 4);
	PIC_sendEOI(12); // 2ch, IRQ12
	fifo8_enqueue(&mousebuf, data);
	return;
}

/*
 * [IN] uint8_t scancode
 * [IN, OUT] MOUSE_DATA_BUNDLE *m
 * FIXME This is broken AF;
 * Many packets are discarded.
 * Because sometimes there may be only 1~2 bytes (QEMU bug?)
 * i.e. when a change is tiny, the 0x08 is not followed by 0x00 0x00
 * Need a Timer
 */
void ps2mouse_decode(uint8_t scancode, MOUSE_DATA_BUNDLE *m)
{
	if (scancode == 0xfa)
		m->phase = 1;
	if (m->phase == 0)
		return;
	/* xy overflow bits not set, bit 3 is set */
	// if (m->phase == 1 && ((scancode & 0xc8) == 0x08))
	/*
	 * xy overflow bits not set, bit 3 is set
	 * And reset the phase on 0x08, 0x18, 0x28... packets
	 * Because previous packet is likely not completed
	 */
	if ((scancode & 0xc8) == 0x08)
	{
		m->buf[0] = scancode;
		m->phase = 2;
		return;
	}

	if (m->phase == 2)
	{
		m->buf[1] = scancode;
		m->phase = 3;
		return;
	}
	if (m->phase == 3)
	{
		m->buf[2] = scancode;
		ps2mouse_parse_three_bytes(m);
		m->phase = 1;
		char _p[20]={0};
		sprintf(_p, "%02d %02d %02x %02x %02x \n", m->x, m->y, m->buf[0], m->buf[1], m->buf[2]);
		kfprint(_p, 4);
		return;
	}
}

static int32_t parse_twos_compliment(int32_t signed_bit, uint8_t tail)
{
	int32_t res = tail;
	if (signed_bit)
	{
		res = ~res + 1;
	}
	// char buf[20]={0};
	// sprintf(buf, "sign:%x tail:%02x res:%02d \n", signed_bit, tail, res);
	// kfprint(buf, 4);
	return res;
}

/*
 * Assume a "second_byte" is already in two's compliment format.
 * i.e. signed == 1, second_byte = 0xfe means -2
 */

static int32_t glue_twos_compliment_fragment(int32_t signed_bit, uint8_t tail)
{
	int32_t res = 0;
	if (signed_bit)
	{
		res = ~res;
		res &= 0xffffff00;
		res |= tail;
	} else
	{
		res = tail;
	}
	return res;
}

int32_t ps2mouse_parse_three_bytes(MOUSE_DATA_BUNDLE *m)
{
	if (!m)
		return -EIO;
	uint8_t state = m->buf[0];
	uint8_t second_byte = m->buf[1];
	uint8_t third_byte = m->buf[2];

	int32_t rel_x = glue_twos_compliment_fragment(isMaskBitsAllSet(state, 0x10), second_byte);
	int32_t rel_y = glue_twos_compliment_fragment(isMaskBitsAllSet(state, 0x20), third_byte);
	// int32_t rel_x = parse_twos_compliment(isMaskBitsAllSet(state, 0x10), second_byte);
	// int32_t rel_y = parse_twos_compliment(isMaskBitsAllSet(state, 0x20), third_byte);
	/*
	 * This only works for int8_t rel_x, rel_y. Because 2 - 0x100 is -254, which is then not properly casted(? it should work?)
	 *
	 * ((state << 4) & 0x100) equals 0x100 only if the signed
	 * bit (9'th bit stored in the first byte) is set. If the 9'th bit is
	 * set then the value is deemed negative, but the value in second_byte
	 * is not stored in one or two's complement form. It is instead stored
	 * as a positive 8-bit value. So, if second_byte is say a 2 then it
	 * will become 2 minus 0 since the negative (9'th bit) is off. But, if
	 * it is on then it will become 2 minus 0x100 which will produce the
	 * twos complement, or -2. It will also cause the register to be
	 * correctly sign extended no matter its size.
	 */
	// int32_t rel_x = second_byte - ((state << 4) & 0x100);
	// int32_t rel_y = third_byte - ((state << 3) & 0x100);
	m->x = rel_x;
	m->y = rel_y;
	return 0;
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

///*
// * Application should handle the buffer?
// * TODO Timeout?
// */
void int21h()
{
	uint8_t volatile data = _io_in8(PS2KBC_PORT_DATA_RW);
	PIC_sendEOI(1); // 21h, IRQ1
	int21h_handler(data);
}

/* ?Race condition/Memory corruption? */
void __int21h_buffed()
{
	uint8_t volatile data = _io_in8(PS2KBC_PORT_DATA_RW);
	// char buf[20]={0};
	// sprintf(buf, "_%02x_", (uint8_t)data);
	// kfprint(buf, 4);
	PIC_sendEOI(1); // 21h, IRQ1
	fifo8_enqueue(&keybuf, data);
	return; // ? from sequential to cpu polling buffer. int21h_handler(data); in another thread?
}

/*
 * Keyboard interrupt handler
 * scancode: uint8_t _scancode
 */
void int21h_handler(uint8_t scancode)
{
	//char buf[20]={0};
	//sprintf(buf, "21h_handler:%02x ", scancode);
	//kfprint(buf, 4);
	atakbd_interrupt(scancode);
}

