#ifndef IO_IO_H_
#define IO_IO_H_

#include <stdint.h>

unsigned char insb(uint16_t port); /* Input Byte String From Port */
uint16_t insw(uint16_t port);
unsigned char outb(uint16_t port, unsigned char val); /* Return the char it wrote */
uint16_t outw(uint16_t port, uint16_t val);
void io_wait(void);

#endif

