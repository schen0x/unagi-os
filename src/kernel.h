#ifndef KERNEL_H_
#define KERNEL_H_

#include <stdint.h>
#include <stddef.h>
#include "util/fifo.h"

void kernel_main(void);
void eventloop(void);
void int2ch_handler(uint8_t scancode);
FIFO32* get_fifo32_common(void);

void __tss_switch4_prep(void);
void __tss_b_main(void);

#endif
