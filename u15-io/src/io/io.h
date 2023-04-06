#ifndef IO_IO_H_
#define IO_IO_H_

#include <stdint.h>

uint8_t insb(uint16_t port);
uint16_t insw(uint16_t port);
uint8_t outb(uint16_t port, uint8_t val);
uint16_t outw(uint16_t port, uint16_t val);

#endif

