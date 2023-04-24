#include "pic/pic.h"
#include "io/io.h"
#include <stdint.h>

// For each PCI controller, there are 4 "Initialization Command Words(ICWs)" need to be sent on init
// Then 3 "Operation Command Words(OCWs) can be sent"
#define PIC0_COMMAND	0x20		/* IO base address for master PIC */
#define PIC0_DATA	(PIC0_COMMAND+1)
#define PIC1_COMMAND	0xA0		/* IO base address for slave PIC */
#define PIC1_DATA	(PIC1_COMMAND+1)

#define ICW1_INIT	0x10		/* Flag; Initialization bit, must present */
#define ICW1_ICW4P	0x01		/* Flag; ICW4 will present */
#define ICW1_SINGLE	0x02		/* Flag; Single (default cascade) mode */
#define ICW1_INTERVAL4	0x04		/* Flag; Call address interval 4 (default 8) bytes */
#define ICW1_LEVEL	0x08		/* Flag; Level triggered (default edge) mode */

#define ICW4_8086	0x01		/* 8086/88 (default MCS-80/85) mode */
#define ICW4_AUTO	0x02		/* Auto (default normal) EOI */
#define ICW4_BUF_SLAVE	0x08		/* Buffered mode/slave */
#define ICW4_BUF_MASTER	0x0C		/* Buffered mode/master */
#define ICW4_SFNM	0x10		/* Special fully nested (default not) */

#define PIC_SIG_EOI	0x20		/* End-of-interrupt signal */


// Let PIC know interrupt has been handled
void PIC_sendEOI(unsigned char irq)
{
	if(irq >= 8)
	{
		_io_out8(PIC1_COMMAND,PIC_SIG_EOI);
	}

	_io_out8(PIC0_COMMAND,PIC_SIG_EOI);
}


/*
 * Set new Interrupt Vector Offsets (was 8h and 70h) to PIC interrupts,
 * by re-initialize the PIC controllers.
 * Commonly: PIC_remap(0x20, 0x28);
 *
 * arguments:
 * offset1 - vector offset for master PIC,
 * 	vectors on the master become offset1..offset1+7
 * offset2 - same for slave PIC: offset2..offset2+7
 */
void PIC_remap(int offset0, int offset1)
{
	_io_out8(PIC0_COMMAND, ICW1_INIT | ICW1_ICW4P );  // 0x11; init, icw4 present, cascade, edge mode
	_io_out8(PIC0_DATA, offset0);                 // ICW2: Set the new intterrupt offset for the Master PIC
	_io_out8(PIC0_DATA, 1 << 2);                       // ICW3: Set Master PIC, take slave intterrupt at IRQ2 (let IRQ be 0 based, 0000 0100)
	_io_out8(PIC0_DATA, 1);                       // ICW4: non buffered mode
	_io_out8(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4P);
	_io_out8(PIC1_DATA, offset1);
	_io_out8(PIC1_DATA, 2);				// ICW3: 0b10, set slave intterrupt at IRQ2
	_io_out8(PIC1_DATA, 1);
}

