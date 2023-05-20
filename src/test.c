#include "test.h"
#include "config.h"
#include "util/dlist.h"
#include "util/kutil.h"
#include "util/fifo.h"

bool test_all()
{
	if(!test_dlist())
		return false;
	if (!test_kutil())
		return false;
	if (!test_fifo32())
		return false;
	return true;
}


#include "util/printf.h"
#include "memory/memory.h"
#include "memory/heapdl.h"
#include "include/uapi/bootinfo.h"

bool heap_debug()
{
	uintptr_t mem0 = kmemtest(0x7E00, 0x7ffff);
	uintptr_t mem = kmemtest(OS_HEAP_ADDRESS, 0xbfffffff) / 1024 / 1024; // End at 0x0800_0000 (128MB) in QEMU
	printf("mem_test OK from addr %4x to %4x \n", 0x7E00, mem0);
	printf("mem_test OK,&=%dMB~%dMB\n", OS_HEAP_ADDRESS/1024/1024, mem);
	printf("VRAM: %p\n", ((BOOTINFO*) OS_BOOT_BOOTINFO_ADDRESS)->vram);
	printf("Used: %dKB", k_heapdl_mm_get_usage()/1024);
	int32_t i = 0;
	uintptr_t handlers[30000] = {0};
	for (i = 0; i < 28000; i++)
	{
		handlers[i] = (uintptr_t)kzalloc(2048);
		if (!handlers[i])
		{
			printf("NULL, i: %d", i);
			break;
		}

	}
	for (i = 0; i < 28000; i++)
		kfree((void *)handlers[i]);

	char *a = (char *) kzalloc(10);
	printf("a: %p", a);
	printf("f: %d", k_heapdl_mm_get_free()/1024/1024);
	printf("chunks: %d", debug__k_heapdl_mm_get_chunks());
	printf("fc: %d", debug__k_heapdl_mm_get_chunksfree());

	return true;
}


