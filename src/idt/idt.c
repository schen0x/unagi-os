#include "idt/idt.h"
#include "memory/memory.h"
#include "config.h"
#include "util/kutil.h"
#include "pic/pic.h"
#include "drivers/keyboard.h"
#include "include/uapi/graphic.h"

/* global variable should be tudo initialized with 0 */
static struct InterruptDescriptorTableRegister32 idtr; // static or not, global variable, address loaded into memory, known in linktime.
static struct InterruptDescriptorTable32 idts[OS_IDT_TOTAL_INTERRUPTS];
/* interrupt_pointer_table[i] is the 32-bit pointer of the generated asm function for "Intterrupt<nubmer>"  */
extern uint32_t* interrupt_pointer_table[OS_IDT_TOTAL_INTERRUPTS];

void idt_zero()
{
	char msg[] = "Divide by zero error\n";
	kfprint(msg, 4);
}

//void _interrupt_handler(uint32_t interrupt, struct interrupt_frame* frame)
void _interrupt_handler(uint32_t interrupt, uint32_t frame)
{
	(void)frame;
	PIC_sendEOI((uint8_t)(interrupt & 0xff));
}

void idt_set(int interrupt_number, void* address)
{
	// char msg[2*sizeof(address)];
	// hex_to_ascii(address, msg, sizeof(address));
	// kprint(msg, sizeof(msg), 4);
	/* Set an "Ring 3 Interrupt Gate" interrupt to the `interrupt_number` */
	struct InterruptDescriptorTable32* idt_entry = &idts[interrupt_number];
	idt_entry->offset_1 = (uint16_t)((uint32_t)address & 0x0000ffff);
	idt_entry->selector = GDT_KERNEL_CODE_SEGMENT_SELECTOR;
	idt_entry->zero = 0x0;
	// idt_entry->type_attr = 0xEE; // Ring 3 Intterrupt Gate, 0b1_11_0_1110 (0xEE)
	idt_entry->type_attr = 0x8E; // Ring 0 Intterrupt Gate, 0b1_11_0_1110 (0xEE)
	idt_entry->offset_2 = (uint16_t)(((uint32_t)address >> 16) & 0x0000ffff);
}

void idt_init()
{
	/* The kmemset is not necessary though, because a global variable will be auto initialized to 0 */
	kmemset(idts, 0, sizeof(idts)); // Set the all idt entries to 0
	kmemset(&idtr, 0, sizeof(idtr)); // Set the all idtr entries to 0
	PIC_remap(0x20, 0x28);

	/* Initialize the IDTR */
	idtr.limit = (uint16_t) (sizeof(idts) - 1);
	idtr.base = (uint32_t) idts;
	for (int i = 0; i < OS_IDT_TOTAL_INTERRUPTS; i++)
	{
		idt_set(i, interrupt_pointer_table[i]);
	}

	idt_set(0, idt_zero);
	idt_set(0x21, int21h);
	idt_load(&idtr);
}


	// kfprint("Keyboard pressed!\n", 4);
	// char hex[2] = {0};
	// kfprint(hex_to_ascii(hex, (char*)&keyPressed, 2), 4);
	// kfprint(k, 4);
/* keyPressed is key + \0 */
void _int21h_handler(uint16_t scancode)
{
	atakbd_interrupt(scancode);
	PIC_sendEOI(0x2);
}

// TODO https://elixir.bootlin.com/linux/latest/source/drivers/input/input.c#L424
void input_report_key(uint8_t scancode, uint8_t down)
{
	if(!down)
	{
		return;
	}
	switch(scancode)
	{
		case 2:
			kfprint("1", 4);
			break;
		case 3:
			kfprint("2", 4);
			break;
		case 4:
			kfprint("3", 4);
			break;
		case 30:
			kfprint("a", 4);
			break;
		case 31:
			kfprint("s", 4);
			break;
		case 32:
			kfprint("d", 4);
			break;

	}
	return;
}
