#include "pic/pic.h"
#include "io/io.h"
#include <stdint.h>

#define PIC1		0x20		/* IO base address for master PIC */
#define PIC2		0xA0		/* IO base address for slave PIC */
#define PIC1_COMMAND	PIC1
#define PIC1_DATA	(PIC1+1)
#define PIC2_COMMAND	PIC2
#define PIC2_DATA	(PIC2+1)
#define PIC_EOI		0x20		/* End-of-interrupt command code */

#define ICW1_ICW4	0x01		/* Indicates that ICW4 will be present */
#define ICW1_SINGLE	0x02		/* Single (cascade) mode */
#define ICW1_INTERVAL4	0x04		/* Call address interval 4 (8) */
#define ICW1_LEVEL	0x08		/* Level triggered (edge) mode */
#define ICW1_INIT	0x10		/* Initialization - required! */

#define ICW4_8086	0x01		/* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO	0x02		/* Auto (normal) EOI */
#define ICW4_BUF_SLAVE	0x08		/* Buffered mode/slave */
#define ICW4_BUF_MASTER	0x0C		/* Buffered mode/master */
#define ICW4_SFNM	0x10		/* Special fully nested (not) */

// Let PIC know interrupt has been handled
void PIC_sendEOI(unsigned char irq)
{
	if(irq >= 8)
	{
		_io_out8(PIC2_COMMAND,PIC_EOI);
	}

	_io_out8(PIC1_COMMAND,PIC_EOI);
}

/*
 * TODO
 * Reinitialize the PIC controllers, giving them specified vector offsets
 * rather than 8h and 70h, as configured by default.
 *
 * arguments:
 * offset1 - vector offset for master PIC,
 * 	vectors on the master become offset1..offset1+7
 * offset2 - same for slave PIC: offset2..offset2+7
 */
void PIC_remap(int offset1, int offset2)
{
	int8_t a1, a2;

	a1 = _io_in8(PIC1_DATA);                        // save masks
	a2 = _io_in8(PIC2_DATA);

	_io_out8(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);  // 0x11; starts the initialization sequence (in cascade mode)
	io_wait();
	_io_out8(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);  // 0x11; starts the initialization sequence (in cascade mode)
	io_wait();
	_io_out8(PIC1_DATA, offset1);                 // ICW2: Master PIC vector offset
	io_wait();
	_io_out8(PIC2_DATA, offset2);                 // ICW2: Slave PIC vector offset
	io_wait();
	_io_out8(PIC1_DATA, 4);                       // ICW3: tell Master PIC that there is a slave PIC at IRQ2 (0000 0100)
	io_wait();
	_io_out8(PIC2_DATA, 2);                       // ICW3: tell Slave PIC its cascade identity (0000 0010)
	io_wait();

	_io_out8(PIC1_DATA, ICW4_8086);               // ICW4: have the PICs use 8086 mode (and not 8080 mode)
	io_wait();
	_io_out8(PIC2_DATA, ICW4_8086);
	io_wait();

	_io_out8(PIC1_DATA, a1);   // restore saved masks.
	_io_out8(PIC2_DATA, a2);
}

