#ifndef CONFIG_H_
#define CONFIG_H_

#define OS_BOOT_BOOTINFO_ADDRESS 0x0ff0
#define OS_GDT_KERNEL_CODE_SEGMENT_SELECTOR 2 * 8
#define OS_GDT_KERNEL_DATA_SEGMENT_SELECTOR 1 * 8 // because last 3 bits are flags
#define OS_IDT_TOTAL_INTERRUPTS 256
#define OS_IDT_AR_INTGATE32 0x8e // access bit for intgate32
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define OS_HEAP_ADDRESS 0x01000000 // Extended Memory; 0x0100_0000 - 0xC000_0000; 3056 MB;
#define OS_HEAP_SIZE_BYTES 1024 * 1024 * 112 // 128MB - OS_HEAP_ADDRESS = 112 MB
#define OS_HEAP_BLOCK_SIZE 4096
// #define OS_SHEAP_TABLE_ADDRESS
#define OS_DISK_SECTOR_SIZE 512 // must be 512 or a multiple of 512
#define OS_PATH_MAX_LENGTH 4096

// #define OS_HEAP_TABLE_ADDRESS 0x00009000 // FIXME reuse 0x7c00 (below 0x8c00) cause panic. Why? 0x00080000 (EBDA) overwritten?
#define OS_HEAP_TABLE_ADDRESS 0x00007c00
#endif
