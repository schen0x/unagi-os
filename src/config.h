#ifndef CONFIG_H_
#define CONFIG_H_

#define GDT_KERNEL_CODE_SEGMENT_SELECTOR 0x08
#define GDT_KERNEL_DATA_SEGMENT_SELECTOR 0x10
#define OS_IDT_TOTAL_INTERRUPTS 256
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
// #define OS_HEAP_ADDRESS 0x00100000
// #define OS_HEAP_SIZE_BYTES 1024 * 1024 * 16
#define OS_HEAP_ADDRESS 0x02000000
#define OS_HEAP_SIZE_BYTES 1024 * 1024 * 50
#define OS_HEAP_BLOCK_SIZE 4096

#endif
