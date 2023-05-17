#include "kernel/process.h"
#include "pic/timer.h"
#include "io/io.h"
#include "gdt/gdt.h"
#include "memory/memory.h"

#include <stdint.h>
#include <stdbool.h>

#define TSS3_GDT_INDEX 3
#define TSS4_GDT_INDEX 4


TSS32 tss_a, tss_b;
int32_t tss_index;
TIMER *tss_switch = NULL;


TASKCTL *taskctl;
TIMER *task_timer;

TSS32* process_gettssa(void)
{
	return &tss_a;
}

TSS32* process_gettssb(void)
{
	return &tss_b;
}


bool process_tss_init(TSS32 *tss)
{
	// TODO
	(void) tss;
	return false;
}

/**
 * Switch Task with the index of TSS in GDT
 * process_switch_by_cs_index(3); => CS_SELECTOR == 3 << 3
 */
void process_switch_by_cs_index(int64_t cs_index)
{
	/* 0x1fff == 0xffff >> 3 */
	uint16_t s = cs_index & 0x1fff;
	_farjmp(0, s * 8);
	return;
}

/**
 * @delay_ms must > 10ms
 */
void process_autotaskswitch(uint32_t delay_ms)
{
	/* Timer must exist */
	if (!task_timer)
		return;
	bool isCli = io_get_is_cli();
	if (!isCli)
		_io_cli();
	timer_settimer(task_timer, delay_ms / 10, 0);
	if (tss_index == TSS3_GDT_INDEX)
	{
		tss_index = TSS4_GDT_INDEX;

	} else {
		tss_index = TSS3_GDT_INDEX;
	}
	_farjmp(0, tss_index * 8);
	if (!isCli)
		_io_sti();
	return;
}

void process_autotaskswitch_init(void)
{
	// tss_switch = timer_alloc_customfifobuf(NULL);
	tss_switch = timer_alloc();
	// timer_settimer(tss_switch, 300, 0);
	timer_settimer(tss_switch, 300, 97);
	return;
}

/**
 * Maybe NULL
 */
TIMER* process_get_tss_timer(void)
{
	// return tss_switch;
	return task_timer;
}

/**
 * Should be called after the gdtr migration
 *   - Append GDT1 with 1000 TSS segments
 *   - ltr the first
 *   - Set the initial task_timer with NULL fifo
 *   - Return the first TASK
 */
TASK *mprocess_init()
{
	TASK *task;
	GDTR32 *gdtr = gdt_get_gdtr();
	GDT32SD *gdts = gdt_get_gdts();
	taskctl = (TASKCTL *) kzalloc(sizeof(TASKCTL));
	for (int32_t i = 0; i < OS_MPROCESS_TASK_MAX; i++)
	{
		taskctl->tasks0[i].flags = 0;
		taskctl->tasks0[i].gdtSegmentSelector = (OS_MPROCESS_TSS_GDT_INDEX_START + i) * 8;
		GDT32SD sd = {0};
		/* Use the recommended magic access_byte 0x89 */
		gdt_set_segmdesc(&sd, sizeof(TSS32) - 1, (uint32_t) &taskctl->tasks0[i].tss, 0x89);
		gdt_append(gdtr, gdts, &sd);
	}

	bool isCli = io_get_is_cli();
	if (!isCli)
		_io_cli();

	gdt1_reload(gdtr);

	task = task_alloc();
	task->flags = 2;
	taskctl->running = 1;
	taskctl->now = 0;
	taskctl->tasks[0] = task;
	_gdt_load_task_register(task->gdtSegmentSelector);
	task_timer = timer_alloc_customfifobuf(NULL);
	timer_settimer(task_timer, 2, 0);
	if (!isCli)
		_io_sti();
	return task;
}

TASK* task_alloc(void)
{
	for (int32_t i = 0 ; i < OS_MPROCESS_TASK_MAX; i++)
	{
		if (taskctl->tasks0[i].flags == 0)
		{
			TASK *task = &taskctl->tasks0[i];
			task->flags = 1;
 			/* IF = 1 */
			task->tss.eflags = 0x202;
			task->tss.eax = 0;
			task->tss.ecx = 0;
			task->tss.edx = 0;
			task->tss.ebx = 0;
			task->tss.ebp = 0;
			task->tss.esi = 0;
			task->tss.edi = 0;
			task->tss.es = 0;
			task->tss.ds = 0;
			task->tss.fs = 0;
			task->tss.gs = 0;
			task->tss.ldtr = 0;
			task->tss.iopb = 0x40000000;
			return task;
		}
	}
	return NULL;
}

/**
 * Switch to and run a `task`
 */
void task_run(TASK *task)
{
	task->flags = 2;
	taskctl->tasks[taskctl->running] = task;
	taskctl->running++;
	return;
}

void task_switch(void)
{
	timer_settimer(task_timer, 2, 0);
	if (taskctl->running >= 2)
	{
		taskctl->now++;
		/* Wrap to 0 */
		if (taskctl->now == taskctl->running)
		{
			taskctl->now = 0;
		}
		_farjmp(0, taskctl->tasks[taskctl->now]->gdtSegmentSelector);
	}
	return;
}

