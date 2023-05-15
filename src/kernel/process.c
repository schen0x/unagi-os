#include "kernel/process.h"
#include <stdint.h>
#include <stdbool.h>
#include "pic/timer.h"

#define TSS3_SEGMENT_SELECTOR 3 * 8
#define TSS4_SEGMENT_SELECTOR 4 * 8
TSS32 tss_a, tss_b;

uint16_t TSSTHIS = TSS3_SEGMENT_SELECTOR;
TIMER *ttss = NULL;

TSS32* process_gettssa(void)
{
	return &tss_a;
}

TSS32* process_gettssb(void)
{
	return &tss_b;
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
void process_autotaskswitch(int32_t delay_ms)
{
	if (TSSTHIS == TSS3_SEGMENT_SELECTOR)
	{
		TSSTHIS = TSS4_SEGMENT_SELECTOR;

	} else
	{
		TSSTHIS = TSS3_SEGMENT_SELECTOR;

	}
	timer_settimer(ttss, delay_ms / 10, 0);
	process_switch_by_cs_index(TSSTHIS / 8);
	return;
}

void process_autotaskswitch_init(void)
{
	ttss = timer_alloc_customfifobuf(NULL);
	timer_settimer(ttss, 300, 0);
	return;
}

TIMER* process_get_ttss(void)
{
	return ttss;
}
