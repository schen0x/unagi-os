/**
 * @file paging.hpp
 *
 * Paging
 */

#pragma once

#include <cstddef>

/** @brief Number of Page Directories (level 2) to be statically reserved
 *
 * Used by SetupIdentityPageMap
 *
 * 2MB per Page * 512 Pages * 64 Page Directories == 64 GB of memory (virtual)
 *
 * Error:
 * (qemu) info mem
 * 0000000000000000-000000003f800000 000000003f800000 -rw
 * 000000003f800000-000000003fe00000 0000000000600000 -r-
 * 000000003fe00000-0000010000000000 000000ffc0200000 -rw
 * (qemu) info mem
 * 0000000000000000-0000001000000000 0000001000000000 -rw
 */
const size_t kPageDirectoryCount = 512;

/** @brief 仮想アドレス=物理アドレスとなるようにページテーブルを設定する．
 * 最終的に CR3 レジスタが正しく設定されたページテーブルを指すようになる．
 */
void SetupIdentityPageTable();
