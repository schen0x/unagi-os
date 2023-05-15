#include "kernel/process.h"
#include <stdint.h>
#include <stdbool.h>

TSS32 tss_a, tss_b;

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
