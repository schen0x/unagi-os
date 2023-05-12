#ifndef KERNEL_PROCESS_H_
#define KERNEL_PROCESS_H_

#include <stdint.h>
/**
 * Task Status Segment
 */
typedef struct TSS32 {
	/* Previous Task Link Field. Contains the Segment Selector for the TSS of the previous task */
	int32_t link;
	/* The Stack Pointers used to load the stack when a privilege level change occurs from a lower privilege level to a higher one */
	int32_t esp0;
	/* The Segment Selectors used to load the stack when a privilege level change occurs from a lower privilege level to a higher one */
	int32_t ss0;
	/* The Stack Pointers used to load the stack when a privilege level change occurs from a lower privilege level to a higher one */
	int32_t esp1;
	/* The Segment Selectors used to load the stack when a privilege level change occurs from a lower privilege level to a higher one */
	int32_t ss1;
	/* The Stack Pointers used to load the stack when a privilege level change occurs from a lower privilege level to a higher one */
	int32_t esp2;
	/* The Segment Selectors used to load the stack when a privilege level change occurs from a lower privilege level to a higher one */
	int32_t ss2;
	int32_t cr3;
	int32_t eip, eflags, eax, ecx, edx, ebx;
	int32_t esp, ebp, esi, edi;
	int32_t es, cs, ss, ds, fs, gs;
	int32_t ldtr;
	/**
	 * I/O Map Base Address Field. Contains a 16-bit offset from the base
	 * of the TSS to the I/O Permission Bit Map.
	 *
	 * So a I/O port is uint16_t, 1 bit per port = 16-bit permission table
	 */
	int32_t iopb;
	/* Shadow Stack Pointer. A defence against stack Overflow */
	int32_t ssp;
} __attribute__((packed)) TSS32;


#endif
