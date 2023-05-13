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

