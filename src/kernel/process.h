#ifndef _KERNEL_PROCESS_H_
#define _KERNEL_PROCESS_H_

#include "config.h"
#include <stdint.h>
#include <stdbool.h>

#define MPROCESS_FLAGS_FREE 0
#define MPROCESS_FLAGS_ALLOCATED 1
#define MPROCESS_FLAGS_RUNNING 2

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

typedef struct TASK
{
	uint32_t gdtSegmentSelector;
	uint32_t flags;
	TSS32 tss;
} TASK;

typedef struct TASKCTL
{
	/* Sum of active tasks */
	int32_t running;
	/* Current running TASK *task = taskctl.tasks[taskctl.now] */
	int32_t now;
	TASK *tasks[OS_MPROCESS_TASK_MAX];
	TASK tasks0[OS_MPROCESS_TASK_MAX];
} TASKCTL;

extern void _farjmp(uint32_t eip, uint16_t cs);

//TIMER* mprocess_get_task_autoswitch_timer(void);
TASK *mprocess_init(void);
TASK *mprocess_task_alloc(void);
void mprocess_task_run(TASK *task);
void mprocess_task_autoswitch(void);
void mprocess_task_sleep(TASK *task);
static void mprocess_save_segment_registers(TASK *tNow);
#endif
