#include "kernel/process.h"
#include "pic/timer.h"
#include "io/io.h"

#include <stdint.h>
#include <stdbool.h>

#define TSS3_GDT_INDEX 3
#define TSS4_GDT_INDEX 4

TSS32 tss_a, tss_b;
int32_t tss_index;
TIMER *tss_switch = NULL;

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
	if (!tss_switch)
		return;
	bool isCli = io_get_is_cli();
	if (!isCli)
		_io_cli();
	timer_settimer(tss_switch, delay_ms / 10, 0);
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
	return tss_switch;
}
