#ifndef CONFIG_H_
#define CONFIG_H_

#define OS_NAME "placeholder"
/* **GDTR0 */
#define OS_BOOT_GDTR0 0x0fe8
#define OS_BOOT_BOOTINFO_ADDRESS 0x0ff0
#define OS_GDT_KERNEL_CODE_SEGMENT_SELECTOR 2 * 8
#define OS_GDT_KERNEL_DATA_SEGMENT_SELECTOR 1 * 8 // because last 3 bits are flags
#define OS_IDT_TOTAL_INTERRUPTS 256
#define OS_IDT_AR_INTGATE32 0x8e // access bit for intgate32
/* Max process allowed when multi-tasking */
#define OS_MPROCESS_TASK_MAX 1000
/* The first TSS segment in GDT */
#define OS_MPROCESS_TSS_GDT_INDEX_START 3
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define OS_MEMORY_ALIGN 16
#define OS_HEAP_ADDRESS 0x01000000 // Extended Memory; 0x0100_0000 - 0xC000_0000; 3056 MB;
#define OS_HEAP_SIZE_BYTES 1024 * 1024 * 112 // 128MB - OS_HEAP_ADDRESS = 112 MB
#define OS_HEAP_BLOCK_SIZE 4096
#define OS_DISK_SECTOR_SIZE 512 // must be 512 or a multiple of 512
#define OS_PATH_MAX_LENGTH 4096

#define OS_HEAP_TABLE_ADDRESS 0x00008e00 // FIXME reuse 0x7c00 (below 0x8c00) cause panic. Why? 0x00080000 (EBDA) overwritten? (Probably Because the "heaptable" heap implementation is bugged.)
// #define OS_HEAP_TABLE_ADDRESS 0x00007c00
#define OS_VGA_MAX_SHEETS 256
#define OS_HEAP_MM_ALT 0x0 // 0x1 to use heap table mm

/* Max (total) numbers of timer that can exist in the OS */
#define OS_MAX_TIMER 500

#define OS_MPROCESS_TASKLEVELS_MAX 10
#define OS_MPROCESS_TASKLEVEL_TASKS_MAX 100

/* Max bytes can present in one line (line buffer) */
# define OS_TEXTBOX_LINE_BUFFER_SIZE 4096 * 10

#define DEV_FIFO_KBD_START 256 // Inclusive; In fifo, data - 256 == keyboard scancode
#define DEV_FIFO_KBD_END 256 + 0xff + 1 // 512, Exclusive; In fifo, data - 256 == keyboard scancode
					//
#define DEV_FIFO_MOUSE_START 512 // Inclusive; In fifo, data - 256 == keyboard scancode
#define DEV_FIFO_MOUSE_END 512 + 0xff + 1 // 768, Exclusive; In fifo, data - 256 == keyboard scancode


#define DEBUG_NO_TIMER 0
#define DEBUG_NO_MULTITASK 0
#endif

