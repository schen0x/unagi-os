#ifndef IO_IO_H_
#define IO_IO_H_

#include <stdint.h>

void io_wait(void);
void _asm_write_mem8(uint32_t addr, uint8_t data);
void _io_hlt(void);
void _io_cli(void);
void _io_sti(void);
void _io_stihlt(void);

/* Read Byte From Port */
uint8_t _io_in8(uint16_t port);
uint16_t _io_in16(uint16_t port);
uint32_t _io_in32(uint16_t port);

/* Write Byte to Port, Return the val write */
uint8_t _io_out8(uint16_t port, uint8_t val);
/* Write 2 Bytes to Port, Return the val write */
uint16_t _io_out16(uint16_t port, uint16_t val);
/* Write 4 Bytes to Port, Return the val write */
uint32_t _io_out32(uint16_t port, uint32_t val);

/* Return the eflags */
uint32_t _io_get_eflags(void);
/* Set the eflags */
uint32_t _io_set_eflags(uint32_t eflags);

#endif

