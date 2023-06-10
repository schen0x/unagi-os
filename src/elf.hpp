#ifndef _ELF_HPP_
#define _ELF_HPP_

#include <stdint.h>

typedef struct ELF64_HEADER
{
  unsigned char e_ident[16];
  uint16_t e_type;
  uint16_t e_machine;
  uint32_t e_version;
  uint64_t e_entry;
  uint64_t e_phoff;
  uint64_t e_shoff;
  uint32_t e_flags;
  uint16_t e_ehsize;
  uint16_t e_phentsize;
  uint16_t e_phnum;
  uint16_t e_shentsize;
  uint16_t e_shnum;
  uint16_t e_shstrndx;
} ELF64_HEADER;

/**
 * ELF Program Header
 * It is found at file offset e_phoff, and consists of e_phnum entries, each
 * with size e_phentsize.
 */
typedef struct ELF64_PGN_HEADER
{
  /* 0x00000001	PT_LOAD	Loadable segment. */
  uint32_t p_type;
  /**
   * Segment-dependent flags (position for 64-bit structure).
   * PF_X 1, PF_W 2, PF_R 4 => .text R E == 0x5
   */
  uint32_t p_flags;
  /* Offset of the segment in the file image. */
  uint64_t p_offset;
  /* Virtual address of the segment in memory. */
  uint64_t p_vaddr;
  /* On systems where physical address is relevant, reserved for segment's
   * physical address. */
  uint64_t p_paddr;
  /* Size in bytes of the segment in the file image. May be 0. */
  uint64_t p_filesz;
  /* Size in bytes of the segment in memory. May be 0. */
  uint64_t p_memsz;
  /* 0 and 1 specify no alignment. Otherwise should be a positive, integral
   * power of 2, with p_vaddr equating p_offset modulus p_align. */
  uint64_t p_align;
} ELF64_PGN_HEADER;

#define ELFFLAGS_PGN_PT_NULL 0
#define ELFFLAGS_PGN_PT_LOAD 1
#define ELFFLAGS_PGN_PT_DYNAMIC 2
#define ELFFLAGS_PGN_PT_INTERP 3
#define ELFFLAGS_PGN_PT_NOTE 4
#define ELFFLAGS_PGN_PT_SHLIB 5
#define ELFFLAGS_PGN_PT_PHDR 6
#define ELFFLAGS_PGN_PT_TLS 7

#endif
