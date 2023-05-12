#include "gdt/gdt.h"
#include "config.h"
#include "util/kutil.h"
#include "kernel/process.h"

static GDTR32 gdtr = {0};
static GDT32 gdts[8192] = {0}; // 64KB (8 bytes each) in .data
TSS32 tss_a, tss_b;

void gdt_set_segmdesc(GDT32 *sd, uint32_t limit, uint32_t base, uint8_t ar)
{
	sd->flags = 0xc;
	sd->limit_low = limit & 0xffff;
	sd->base_low = base & 0xffff;
	sd->base_mid = (base >> 16) & 0xff;
	sd->access_byte = ar & 0xff;
	sd->limit_high = (limit >> 16) & 0x0f;
	sd->base_high = (base >> 24) & 0xff;
	return;
}

/**
 * Copy all gdt entries from src to dst
 * According to the original GDTR
 * WARN: Do not overflow the gdtsDst
 * [IN] gdtrSrc
 * [OUT] gdtrDst
 * [OUT] gdtsDst
 */
void __gdt_import(GDTR32 *gdtrDst, GDT32 *gdtsDst, GDTR32 *gdtrSrc)
{
	gdtrDst->base = (uint32_t) gdtsDst;
	gdtrDst->limit = gdtrSrc->limit;
	GDT32 *gdtsSrc = (GDT32 *)(uintptr_t) gdtrSrc->base;
	kmemcpy(gdtsDst, gdtsSrc, (gdtrSrc->limit + 1) * 8);
}
/**
 * Call this sooner than later because those unmanaged memory may get overwritten
 */
void gdt_read_gdtr0()
{
	__gdt_import(&gdtr, gdts, (GDTR32 *)(uintptr_t) OS_BOOT_GDTR0);
}


/**
 * Append 1 GDT32 descriptor to the existing entries
 * @d1 GDT32 d1[1];
 * Return: the Segment Selector (d_len * 8), (MAX 8192)
 */
uint16_t gdt_append(GDTR32 *r, GDT32 *d, GDT32 *d1)
{
	uint32_t d_len = r->limit + 1;
	kmemcpy(&d[d_len], d1, sizeof(GDT32));
	r->limit += 1;
	return d_len << 3;
}

void gdt_tss_init()
{
	gdt_read_gdtr0();
	tss_a.ldtr = 0;
	tss_a.iopb = 0x40000000;
	tss_b.ldtr = 0;
	tss_b.iopb = 0x40000000;
	GDT32 tss_a_seg = {0};
	GDT32 tss_b_seg = {0};
	/* Use the recommended magic access_byte 0x89 */
	gdt_set_segmdesc(&tss_a_seg, sizeof(TSS32) - 1, (uint32_t) &tss_a, 0x89);
	gdt_set_segmdesc(&tss_b_seg, sizeof(TSS32) - 1, (uint32_t) &tss_b, 0x89);
	uint16_t tss_a_segment_selector = gdt_append(&gdtr, gdts, &tss_a_seg);
	gdt_append(&gdtr, gdts, &tss_b_seg);
	/**
	 * Use the new gdtr location && content
	 * The CS, DS can be the same because we copied the original gdtr
	 * thus the original Selector does no change
	 */
	_gdt_load(&gdtr);
	_gdt_load_task_register(tss_a_segment_selector);
}


