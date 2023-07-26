/**
 * "Ordinary 4-Level Paging" (SDM 1-4, p3120)
 * - 4-level; max 48-bit linear addresses (256TB)
 * - orginary paging (use CR3 to locate the first paging structure)
 * - Use 2M Page
 */
#include "paging.hpp"

#include <array>

#include "asmfunc.h"
#include <stdint.h>

namespace
{
const uint64_t kPageSize4K = 4096;
const uint64_t kPageSize2M = 4096 * 512;
const uint64_t kPageSize1G = 4096 * 512 * 512;

/* Page Map Level 4 Table (PML4E) */
alignas(kPageSize4K) std::array<uint64_t, 512> PML4E;
/* Page directory Pointer Table (PDPTE, level 3) */
alignas(kPageSize4K) std::array<uint64_t, 512> PDPTE;
/* Page Directory (PDE, level 2) */
//! alignas(kPageSize4K) std::array<std::array<uint64_t, 512>, kPageDirectoryCount> PDE;
alignas(kPageSize4K) std::array<std::array<uint64_t, 512>, 512> PDE;
} // namespace

/**
 * - Setup PML4E[0] so that logical == physical address
 * - Enable (CR3 == &PML4E) (For 4-level paging, ... (CR3) is the PML4 table)
 */
void SetupIdentityPageTable()
{
  for (uint64_t i_PML4E = 0; i_PML4E < 512; ++i_PML4E)
  {
    PML4E[i_PML4E] = reinterpret_cast<uint64_t>(&PDPTE[i_PML4E]) | 0x003;
    /* Lv3 PDPTE */
    for (uint64_t i_PDPTE = 0; i_PDPTE < 512; ++i_PDPTE)
    {
      PDPTE[i_PDPTE] = reinterpret_cast<uint64_t>(&PDE[i_PDPTE]) | 0x003;
      for (int i_pd = 0; i_pd < 512; ++i_pd)
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
        PDE[i_PDPTE][i_pd] = i_PDPTE * kPageSize1G + i_pd * kPageSize2M | 0x083;
      }
    }
  }

  SetCR3(reinterpret_cast<uint64_t>(&PML4E[0]));
  // SetCR3(reinterpret_cast<uint64_t>(&pml4_table));
}
