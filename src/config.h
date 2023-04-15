#ifndef CONFIG_H_
#define CONFIG_H_

#define GDT_KERNEL_CODE_SEGMENT_SELECTOR 0x08
#define GDT_KERNEL_DATA_SEGMENT_SELECTOR 0x10
#define OS_IDT_TOTAL_INTERRUPTS 256
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define OS_HEAP_ADDRESS 0x01000000 // Extended Memory; 0x0100_0000 - 0xC000_0000; 3056 MB;
#define OS_HEAP_SIZE_BYTES 1024 * 1024 * 100 // 100MB
#define OS_HEAP_BLOCK_SIZE 4096
// #define OS_SHEAP_TABLE_ADDRESS
#define OS_DISK_SECTOR_SIZE 512 // must be 512 or a multiple of 512
#define OS_PATH_MAX_LENGTH 4096
#endif
