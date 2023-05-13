#ifndef KERNEL_PROCESS_H_
#define KERNEL_PROCESS_H_

#include <stdint.h>
#include <stdbool.h>
/**
 * Task Status Segment
 */
typedef struct TSS32 {
	/* Previous Task Link Field. Contains the Segment Selector for the TSS of the previous task */
	uint32_t link;
	/* The Stack Pointers used to load the stack when a privilege level change occurs from a lower privilege level to a higher one */
	uint32_t esp0;
	/* The Segment Selectors used to load the stack when a privilege level change occurs from a lower privilege level to a higher one */
	uint32_t ss0;
	/* The Stack Pointers used to load the stack when a privilege level change occurs from a lower privilege level to a higher one */
	uint32_t esp1;
	/* The Segment Selectors used to load the stack when a privilege level change occurs from a lower privilege level to a higher one */
	uint32_t ss1;
	/* The Stack Pointers used to load the stack when a privilege level change occurs from a lower privilege level to a higher one */
	uint32_t esp2;
	/* The Segment Selectors used to load the stack when a privilege level change occurs from a lower privilege level to a higher one */
	uint32_t ss2;
	uint32_t cr3;
	uint32_t eip, eflags, eax, ecx, edx, ebx;
	uint32_t esp, ebp, esi, edi;
	uint32_t es, cs, ss, ds, fs, gs;
	uint32_t ldtr;
	/**
	 * I/O Map Base Address Field. Contains a 16-bit offset from the base
	 * of the TSS to the I/O Permission Bit Map.
	 *
	 * So a I/O port is uint16_t, 1 bit per port = 16-bit permission table
	 */
	uint32_t iopb;
	/* Shadow Stack Pointer. A defence against stack Overflow */
	uint32_t ssp;
} __attribute__((packed)) TSS32;


TSS32* process_gettssa(void);
TSS32* process_gettssb(void);
bool process_tss_init(TSS32 *tss);
void _farjmp(uint32_t eip, uint16_t cs);
#endif
