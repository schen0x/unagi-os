#include "kernel/process.h"
#include "pic/timer.h"
#include "io/io.h"
#include "gdt/gdt.h"
#include "memory/memory.h"
#include "util/printf.h"

#include <stdint.h>
#include <stdbool.h>

TASKCTL *taskctl;
int32_t __MPGUARD = 0;

/**
 * Should be called after the gdtr migration
 *   - Append GDT1 with 1000 TSS segments
 *   - ltr the first
 *   - Set the initial task_timer with NULL fifo
 *   - Return the first TASK
 */
TASK *mprocess_init(void)
{
	/* Overflow guard */
 	__MPGUARD++;
	if (__MPGUARD > 1)
	{
		printf("__MPGUARD");
		asm("HLT");
	}

	TASK *task;
	GDTR32 *gdtr = gdt_get_gdtr();
	GDT32SD *gdts = gdt_get_gdts();
	taskctl = (TASKCTL *) kzalloc(sizeof(TASKCTL));
	for (int32_t i = 0; i < OS_MPROCESS_TASK_MAX; i++)
	{
		taskctl->tasks0[i].flags = MPROCESS_FLAGS_FREE;
		taskctl->tasks0[i].gdtSegmentSelector = (OS_MPROCESS_TSS_GDT_INDEX_START + i) * 8;
		GDT32SD sd = {0};
		/**
		 * Intel Software Developer Manual, Volume 3-A.
		 * Section 3.4.5: Segment Descriptors:
		 *   - Offsets less than or equal to the segment limit generate
		 * general-protection exceptions or stack-fault exceptions
		 *
		 * The `le` condition indicates the `limit` probably should not -1
		 *
		 * Use the recommended magic access_byte 0x89
		 *
		 */
		gdt_set_segmdesc(&sd, sizeof(TSS32), (uint32_t) &taskctl->tasks0[i].tss, 0x89);
		gdt_append(gdtr, gdts, &sd);
	}

	bool isCli = io_get_is_cli();
	if (!isCli)
		_io_cli();

	gdt1_reload(gdtr);

	task = mprocess_task_alloc();
	task->flags = MPROCESS_FLAGS_RUNNING;
	taskctl->running = 1;
	taskctl->now = 0;
	taskctl->tasks[0] = task;
	_gdt_ltr((uint16_t) task->gdtSegmentSelector);
	TIMER *tssTimer = timer_get_tssTimer();
	timer_settimer(tssTimer, 10, 0);
	if (!isCli)
		_io_sti();
	return task;
}

TASK* mprocess_task_alloc(void)
{
	for (int32_t i = 0 ; i < OS_MPROCESS_TASK_MAX; i++)
	{
		if (taskctl->tasks0[i].flags == MPROCESS_FLAGS_FREE)
		{
			TASK *taskNew = &taskctl->tasks0[i];
			taskNew->flags = MPROCESS_FLAGS_ALLOCATED;
 			/* IF = 1 */
			taskNew->tss.eflags = 0x00000202;
			taskNew->tss.eax = 0;
			taskNew->tss.ecx = 0;
			taskNew->tss.edx = 0;
			taskNew->tss.ebx = 0;
			taskNew->tss.ebp = 0;
			taskNew->tss.esi = 0;
			taskNew->tss.edi = 0;
			taskNew->tss.es = 0;
			taskNew->tss.ds = 0;
			taskNew->tss.fs = 0;
			taskNew->tss.gs = 0;
			taskNew->tss.ldtr = 0;
			taskNew->tss.iopb = 0x40000000;
			return taskNew;
		}
	}
	return NULL;
}

/**
 * Mark an ALLOCATED `task` as RUNNING
 */
void mprocess_task_run(TASK *task)
{
	if (!task)
		return;
	if (task->flags != MPROCESS_FLAGS_ALLOCATED)
		return;
	task->flags = MPROCESS_FLAGS_RUNNING;
	taskctl->tasks[taskctl->running] = task;
	taskctl->running++;
	return;
}

void mprocess_task_autoswitch(void)
{
	TIMER *tssTimer = timer_get_tssTimer();
	timer_settimer(tssTimer, 2, 0);
	if (taskctl->running >= 2)
	{
		taskctl->now++;
		/* Wrap to 0 */
		if (taskctl->now == taskctl->running)
		{
			taskctl->now = 0;
		}
		uint16_t ss = taskctl->tasks[taskctl->now]->gdtSegmentSelector;
		// printf("ss:%d", ss);
		_farjmp(0, ss);
	}
	return;
}

/**
 * Remove a task from the scheduler
 *   - `needTaskSwitchNow` if the task calls for `sleep()`
 *
 * TODO Reimplement; not good code
 */
void mprocess_task_sleep(TASK *task)
{

	int32_t i = 0;
	bool needTaskSwitchNow = false;
	/* Task must be running */
	if (task->flags != MPROCESS_FLAGS_RUNNING)
		return;
	/* The current running task wishes to enter its slumber */
	if (task == taskctl->tasks[taskctl->now])
		needTaskSwitchNow = true;
	/* Find the task */
	for (i = 0; i < taskctl->running; i++)
	{
		if (taskctl->tasks[i] == task)
		{
			break;
		}
	}
	/* No hit; ERROR */
	if (i > taskctl->running)
		return;

	/* Remove the task from the scheduler */
	taskctl->running--;
	if (i < taskctl->now)
		taskctl->now--;
	for (; i< taskctl->running; i++)
		taskctl->tasks[i] = taskctl->tasks[i + 1];
	task->flags = MPROCESS_FLAGS_ALLOCATED;
	if (needTaskSwitchNow != 0)
	{
		taskctl->now++;
		if (taskctl->now >= taskctl->running)
			taskctl->now = 0;
		_farjmp(0, taskctl->tasks[taskctl->now]->gdtSegmentSelector);
	}
}

static void __mprocess_task_idle(void)
{
	for (;;)
	{
		_io_hlt();
	}
}
