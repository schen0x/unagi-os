/**
 * TODO Find a better way to impl
 *   - This is not easily testable and uses too many states
 */
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

	/* Init the current task */
	task = mprocess_task_alloc();
	task->flags = MPROCESS_FLAGS_RUNNING;
	task->priority = 2;
	task->level = 0;
	__mprocess_task_add(task);
	__mprocess_task_update_currentlv();
	_gdt_ltr((uint16_t) task->gdtSegmentSelector);
	TIMER *tssTimer = timer_get_tssTimer();
	timer_settimer(tssTimer, task->priority, 0);

	TASK *taskIdle = mprocess_task_alloc();
	taskIdle->level = OS_MPROCESS_TASKLEVELS_MAX - 1;
	taskIdle->priority = 1;
	taskIdle->tss.esp = (uint32_t) (uintptr_t) kmalloc(4096) + 4096 - 4;
	taskIdle->tss.eip = (uint32_t) &__mprocess_task_idle;
	taskIdle->tss.cs = OS_GDT_KERNEL_CODE_SEGMENT_SELECTOR;
	taskIdle->tss.es = OS_GDT_KERNEL_DATA_SEGMENT_SELECTOR;
	taskIdle->tss.ss = OS_GDT_KERNEL_DATA_SEGMENT_SELECTOR;
	taskIdle->tss.ds = OS_GDT_KERNEL_DATA_SEGMENT_SELECTOR;
	taskIdle->tss.fs = OS_GDT_KERNEL_DATA_SEGMENT_SELECTOR;
	taskIdle->tss.gs = OS_GDT_KERNEL_DATA_SEGMENT_SELECTOR;
	mprocess_task_run(taskIdle, -1, 0);
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
			taskNew->priority = 2;
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
 * @level: if < 0, keep the previous level
 * @priority: if 0, keep the previous priority
 */
void mprocess_task_run(TASK *task, int32_t level, uint32_t priority)
{
	if (!task)
		return;
	if (level < 0)
		level = task->level; /* no change if < 0 */
	/* Update priority; if 0, no change (waking from sleep) */
	if (priority > 0)
		task->priority = priority;
	/* Change level while running */
	if (task->flags == MPROCESS_FLAGS_RUNNING && task->level != level)
	{
		/* will change flag to allocated */
		mprocess_task_remove(task);
	}
	if (task->flags == MPROCESS_FLAGS_ALLOCATED)
	{
		task->level = level;
		__mprocess_task_add(task);
	}
	taskctl->lv_change = true;

	return;
}

/**
 * A timer is triggerred
 * Time to hand the control over, to the next task
 *   - The next timeout is the priority of the next task
 *   - The next task with 0xFFFFFFFF timeout runs forever
 */
void mprocess_task_autoswitch(void)
{
	TIMER *tssTimer = timer_get_tssTimer();
	TASKLEVEL *tl = &taskctl->level[taskctl->now_lv];
	TASK *taskNext = NULL, *taskCurrent = tl->tasks[tl->now];

	/**
	 * A level change happens when higher level tasks[] are depleted
	 * In that case, goto tlNext->tasks[0]
	 */
	tl->now++;
	if (tl->now == tl->running)
		tl->now = 0;
	if (taskctl->lv_change == true)
	{
		__mprocess_task_update_currentlv();
		tl = &taskctl->level[taskctl->now_lv];
	}
	taskNext = tl->tasks[tl->now];
	timer_settimer(tssTimer, taskNext->priority, 0);
	if (taskNext != taskCurrent)
		_farjmp(0, taskNext->gdtSegmentSelector);
	return;
}

/**
 * Remove a task from the scheduler
 *   - do switch if the current task wishes to `sleep()`
 */
void mprocess_task_sleep(TASK *task)
{
	if (!task)
		return;
	if (task->flags != MPROCESS_FLAGS_RUNNING)
		return;
	TASK *taskCurrent = mprocess_task_get_current();
	mprocess_task_remove(task);

	/**
	 * The current running task wishes to enter its slumber
	 *   - do switch
	 */
	if (task == taskCurrent)
	{
		__mprocess_task_update_currentlv();
		taskCurrent = mprocess_task_get_current();
		_farjmp(0, taskCurrent->gdtSegmentSelector);
	}
	return;
}

static void __mprocess_task_idle(void)
{
	for (;;)
	{
		_io_hlt();
	}
}

TASK* mprocess_task_get_current(void)
{
	TASKLEVEL *tl = &taskctl->level[taskctl->now_lv];
	return tl->tasks[tl->now];
}

void __mprocess_task_add(TASK *task)
{
	TASKLEVEL *tl = &taskctl->level[task->level];
	/* Prevent a task to be added multiple times */
	for (int32_t i = 0; i < tl->running; i++)
	{
		if (tl->tasks[i] == task)
			return;
	}
	tl->tasks[tl->running] = task;
	tl->running++;
	task->flags = MPROCESS_FLAGS_RUNNING;
	return;
}

void mprocess_task_remove(TASK *task)
{
	if (!task)
		return;
	int32_t i;
	TASKLEVEL *tl = &taskctl->level[task->level];

	for (i = 0; i < tl->running; i++)
	{
		if (tl->tasks[i] == task)
			break; /* Found */
	}
	tl->running--;

	if (i < tl->now)
		tl->now--;
	if (tl->now >= tl->running)
		tl->now = 0;

	task->flags = MPROCESS_FLAGS_ALLOCATED;

	for (; i < tl->running; i++)
		tl->tasks[i] = tl->tasks[i + 1];
	if (tl->running == 0)
		taskctl->lv_change = true;
	return;

}

void __mprocess_task_update_currentlv(void)
{
	int32_t i = 0;
	for (i = 0; i < OS_MPROCESS_TASKLEVELS_MAX; i++)
	{
		if (taskctl->level[i].running > 0)
			break;
	}
	if (i >= OS_MPROCESS_TASKLEVELS_MAX)
		i = OS_MPROCESS_TASKLEVELS_MAX - 1;
	taskctl->now_lv = i;
	taskctl->lv_change = false;
	return;
}

