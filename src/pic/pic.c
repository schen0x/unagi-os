/* 8259 PIC */
#include "pic/pic.h"
#include "io/io.h"
#include <stdint.h>

// For each PIC controller, there are 4 "Initialization Command Words(ICWs)" need to be sent on init
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

#define PIC_SIG_EOI_ALL	0x20		/* Non-Specific End-of-interrupt signal, OCW2, D5==1 */
#define PIC_SIG_EOI_SP	0x60		/* End-of-interrupt signal, OCW2; IRQ0 is 0x60 | 0, IRQ7 is 0x60 | 7 */


/*
 * Send PIC_SIG_EOI_ALL (0x20) to PIC0_COMMAND or PIC1_COMMAND,
 * to let corresponding PIC to know the interrupt has been handled
 * irq: 0-15
 */
void PIC_sendEOI(uint8_t irq)
{
	// 0:7
	if (irq < 8)
	{
		_io_out8(PIC0_COMMAND, PIC_SIG_EOI_SP | irq); // 0x60 to 0x67, send special EOI of IRQ0 to IRQ7
		return;
	}
	// 8:15
	if(irq < 16 && irq >= 8)
	{
		_io_out8(PIC1_COMMAND,PIC_SIG_EOI_SP | (irq - 8));
		_io_out8(PIC0_COMMAND, PIC_SIG_EOI_SP | 2); // IRQ2
		return;
	}
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
void PIC_remap(uint8_t offset0, uint8_t offset1)
{
	_io_out8(PIC0_COMMAND, ICW1_INIT | ICW1_ICW4P);	// 0x11; init, icw4 present, cascade, edge mode
	_io_out8(PIC0_DATA, offset0);			// ICW2: Set the new interrupt offset for the Master PIC
	_io_out8(PIC0_DATA, 1 << 2);			// ICW3: Set Master PIC, take slave interrupt at IRQ2 (let IRQ be 0 based, 0000 0100)
	_io_out8(PIC0_DATA, ICW4_8086);			// ICW4: non buffered mode
	_io_out8(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4P | ICW1_LEVEL);
	_io_out8(PIC1_DATA, offset1);
	_io_out8(PIC1_DATA, 2);				// ICW3: 0b10, set slave interrupt at IRQ2
	_io_out8(PIC1_DATA, ICW4_8086);

	// Optional
	_io_out8(PIC0_DATA, 0xff ^ ( 1 | 1 << 1 | 1 << 2));	// OCW1(IMR): 1 is "masked"; mask all, except the timer (IRQ0), keyboard interrupt (0x21 (IRQ1)) and PIC1 interrupt (IRQ2)
	_io_out8(PIC1_DATA, 0xff ^ (1 << 4));		// OCW1(IMR): mask all, except IRQ12
}

