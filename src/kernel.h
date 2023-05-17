#ifndef KERNEL_H_
#define KERNEL_H_

#include <stdint.h>
#include <stddef.h>
#include "util/fifo.h"
#include "include/uapi/graphic.h"

void kernel_main(void);
void eventloop(void);
void int2ch_handler(uint8_t scancode);
FIFO32* get_fifo32_common(void);

//void __tss_switch4_prep(uint32_t tss4_esp);
// void __tss_b_main(SHEET* sw);
void __tss_b_main();
int32_t get_guard();

#endif
