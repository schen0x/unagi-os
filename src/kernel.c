#include "kernel.h"
#include "config.h"
#include "util/kutil.h"
#include "util/printf.h"
#include "idt/idt.h"
#include "io/io.h"
#include "memory/memory.h"
#include "memory/kheap.h"
#include "memory/paging/paging.h"
#include "disk/disk.h"
#include "fs/pathparser.h"
#include "include/uapi/graphic.h"
#include "disk/dstream.h"
#include "include/uapi/bootinfo.h"
#include "test.h"

/* Kernel Page Directory */
PAGE_DIRECTORY_4KB* kpd = 0;

void kernel_main(void)
{
	idt_init();
	graphic_initialize((BOOTINFO*) OS_BOOT_BOOTINFO_ADDRESS);

	uintptr_t mem0 = kmemtest(0x7E00, 0x7ffff);
	printf("mem_test OK from addr %4x to %4x \n", 0x7E00, mem0);
	uintptr_t mem = kmemtest(OS_HEAP_ADDRESS, 0xbfffffff) / 1024 / 1024; // End at 0x0800_0000 (128MB) in QEMU
	printf("mem_test OK from addr %dMB to %dMB \n", OS_HEAP_ADDRESS/1024/1024, mem);
	_io_sti();
	asm("int $99");
	// TODO e820 routine

	k_mm_init();
	char *s = (char *)kmalloc(100);
	printf("*s:%p ", s);
	char *s2 = (char *)kmalloc(100);
	printf("*s2:%p ", s2);
	char *s3 = (char *)kmalloc(10);
	printf("*s3:%p ", s3);
	kfree(s2);
	kfree(s3);
	char *s4 = (char *)kmalloc(10);
	printf("*s4:%p ", s4);
	char *s5 = (char *)kmalloc(10);
	printf("*s5:%p ", s5);


	// ==============
	(void) kpd;
//	uint32_t pd_entries_flags = 0b111;
//	kpd = pd_init(pd_entries_flags);
//	paging_switch(kpd);
	//enable_paging();

	disk_search_and_init();

	if (!test_all())
	{
		kfprint("\nFunction test FAIL.", 4);
	} else
	{
		kfprint("\nFunction test PASS.", 4);
	}
	eventloop();
	// asm("hlt");
}

void eventloop(void)
{
	int32_t usedBytes_keybuf, usedBytes_mousebuf = 0;
	for(;;)
	{
		_io_cli();
		usedBytes_keybuf = fifo8_status_getUsageB(&keybuf);
		usedBytes_mousebuf = fifo8_status_getUsageB(&mousebuf);
		if (usedBytes_keybuf == 0 && \
			usedBytes_mousebuf == 0)
		{
			_io_sti();
			asm("pause");
			continue;
		}
		if (usedBytes_keybuf != 0)
		{
			/*
			 * Because if error returns a int32_t negative number,
			 * when autocasted, the sign digit is NOT discarded by default.
			 * uint8_t scancode = (uint32_t) -1;
			 * -> `scancode` becomes 0xff;
			 */
			int32_t kbdscancode = fifo8_dequeue(&keybuf);
			if (kbdscancode >= 0)
				int21h_handler(kbdscancode & 0xff);
		}
		if (usedBytes_mousebuf != 0)
		{
			int32_t mousescancode = fifo8_dequeue(&mousebuf);
			if (mousescancode >= 0)
				int2ch_handler(mousescancode & 0xff);

		}
	}

}


