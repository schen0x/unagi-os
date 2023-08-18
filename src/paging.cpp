/**
 * "Ordinary 4-Level Paging" (SDM 1-4, p3120)
 * - 4-level; max 48-bit linear addresses (256TB)
 * - orginary paging (use CR3 to locate the first paging structure)
 * - Use 2M Page
 * - QEMU 8.0.90
 *   - The root cause of the panic was: fail to setup the "higher address"
 *   (the address range covered by PML4E[1]; the address is used during PCIE setup)
 *   - To debug, see (https://forum.osdev.org/viewtopic.php?f=1&t=40321)
 *   - TL;DR:
 *     1. Understand the debug info, the panic is caused by page fault
 *     2. Use "info tlb" to output the Translation Lookaside Buffer (TLB)
 */
#include "paging.hpp"
#include "asmfunc.h"
#include <array>
#include <stdint.h>

namespace
{
const uint64_t kPageSize4K = 1024 * 4;
const uint64_t kPageSize2M = 1024 * 1024 * 2;
const uint64_t kPageSize1G = 1024 * 1024 * 1024;
const uint64_t kPageSize512G = kPageSize1G * 512;

#define PML4E_SIZE 4
#define PDPTE_SIZE 512
#define PDE_SIZE 512

/* Page Map Level 4 Table (PML4E) */
alignas(kPageSize4K) std::array<uint64_t, PML4E_SIZE> PML4E;
/* Page directory Pointer Table (PDPTE, level 3) */
alignas(kPageSize4K) std::array<std::array<uint64_t, PDPTE_SIZE>, PML4E_SIZE> PDPTE;
/* Page Directory (PDE, level 2) */
//! alignas(kPageSize4K) std::array<std::array<uint64_t, 512>, kPageDirectoryCount> PDE;
alignas(kPageSize4K) std::array<std::array<std::array<uint64_t, PDE_SIZE>, PDPTE_SIZE>, PML4E_SIZE> PDE;
} // namespace

/**
 * - Setup PML4E[0] so that logical == physical address
 * - Enable (CR3 == &PML4E) (For 4-level paging, ... (CR3) is the PML4 table)
 */
void SetupIdentityPageTable()
{
  for (uint64_t i_PML4E = 0; i_PML4E < PML4E_SIZE; ++i_PML4E)
  {
    PML4E[i_PML4E] = reinterpret_cast<uint64_t>(&PDPTE[i_PML4E]) | 0x003;
    /* Lv3 PDPTE */
    for (uint64_t i_PDPTE = 0; i_PDPTE < PDPTE_SIZE; ++i_PDPTE)
    {
      PDPTE[i_PML4E][i_PDPTE] = reinterpret_cast<uint64_t>(&PDE[i_PML4E][i_PDPTE]) | 0x003;
      for (int i_pd = 0; i_pd < PDE_SIZE; ++i_pd)
      {
        /**
         * offset1 (pdp[i_pdp+1] - pdp[i_pdp]) == i_pdp * kPageSize1G
         * offset2 (pd[i_pd+1] - pd[i_pd]) == i_pd * kPageSize2M
         * (Table 4-18. Format of a Page-Directory Entry that Maps a 2-MByte Page, SDM 1-4, p3129)
         * - Flags 0x083
         *   Bit 63 ... 0: 0b...10000011
         *   0: true; Present
         *   1: true; Allow write
         *   7: true; Page size, (true for 2M pages)
         *   ...
         *   (M: MAXPHYADDR is the maximum physical address size and is indicated by CPUID.80000008H:EAX[bits 7-0].)
         *   M-21: physical address
         *
         *   0x80'0000'0000 == kPageSize1G * 512
         */
        //? page_directory[i_pdp][i_pd] = 0xFFFF'FF80'0000'0000 + i_pdp * kPageSize1G + i_pd * kPageSize2M | 0x083;
        PDE[i_PML4E][i_PDPTE][i_pd] = i_PML4E * kPageSize512G + i_PDPTE * kPageSize1G + i_pd * kPageSize2M | 0x083;
      }
    }
  }

  SetCR3(reinterpret_cast<uint64_t>(&PML4E[0]));
  // SetCR3(reinterpret_cast<uint64_t>(&pml4_table));
}
