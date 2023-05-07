#ifndef IO_IO_H_
#define IO_IO_H_

#include <stdint.h>
#include <stdbool.h>

/* Carry flag */
#define FLAGS_MASK_CF 0x0001
/* Parity flag */
#define FLAGS_MASK_PF 0x0004
/* Auxiliary Carry flag */
#define FLAGS_MASK_AF 0x0010
/* Zero flag */
#define FLAGS_MASK_ZF 0x0040
/* Sign flag */
#define FLAGS_MASK_SF 0x0080
/* Interrupt Enable flag */
#define FLAGS_MASK_IF 0x0200
/* Direction flag */
#define FLAGS_MASK_DF 0x0400
/* Overflow flag */
#define FLAGS_MASK_OF 0x0800
/* Alignment Check (486+, ring 3) flag */
#define EFLAGS_MASK_AC 0x00040000
/* Virtual Interrupt (Pentium+) flag */
#define EFLAGS_MASK_VIF 0x00080000
/* Virtual Interrupt Pending (Pentium+) flag */
#define EFLAGS_MASK_VIP 0x00100000
/* Can use CPUID (Pentium+) flag */
#define EFLAGS_MASK_ID 0x00200000

/* Protected Mode Enabled */
#define CR0_MASK_PE 1 << 0
/* Not Write-through */
#define CR0_MASK_NW 1 << 29
/* Cache Disable */
#define CR0_MASK_CD 1 << 30
/* Paging Enabled, If 1, enable paging and use the CR3 register. */
#define CR0_MASK_PG 1 << 31

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
/* Return the cr0 flags */
uint32_t _io_get_cr0(void);
/* Set the cr0 flags */
uint32_t _io_set_cr0(uint32_t cr0);
bool io_get_is_cli(void);

#endif

