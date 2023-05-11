#ifndef KERNEL_H_
#define KERNEL_H_

#include <stdint.h>
#include <stddef.h>

void kernel_main(void);
void eventloop(void);
void int2ch_handler(uint8_t scancode);

#endif
