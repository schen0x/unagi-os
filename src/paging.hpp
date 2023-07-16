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
 */
const size_t kPageDirectoryCount = 64;

/** @brief 仮想アドレス=物理アドレスとなるようにページテーブルを設定する．
 * 最終的に CR3 レジスタが正しく設定されたページテーブルを指すようになる．
 */
void SetupIdentityPageTable();
