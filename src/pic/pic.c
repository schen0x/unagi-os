#include "pic/pic.h"
#include "io/io.h"
#include <stdint.h>

// Let PIC know interrupt has been handled
void PIC_sendEOI(unsigned char irq)
{
	if(irq >= 8)
	{
		_io_out8(PIC2_COMMAND,PIC_EOI);
	}

	_io_out8(PIC1_COMMAND,PIC_EOI);
}

/* Reinitialize the PIC controllers, giving them specified vector offsets
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

	_io_out8(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);  // starts the initialization sequence (in cascade mode)
	io_wait();
	_io_out8(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
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

