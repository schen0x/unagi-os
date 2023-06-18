/**
 * @file usb/xhci/ring.hpp
 *
 * A Ring is a circular queue of data structures. Three types of Rings are used by the xHC to communicate and execute
 * USB operations:
 *   - Command Ring: One for the xHC
 *   - Event Ring: One for each Interrupter (refer to section 4.17)
 *   - Transfer Ring: One for each Endpoint or Stream
 *   The Command Ring is used by system software to issue commands to the xHC.
 *   The Event Ring is used by the xHC to return status and results of commands and transfers to system software.
 *   Transfer Rings are used to move data between system memory buffers and device endpoints.
 */

#pragma once

#include <cstdint>
#include <vector>

#include "error.hpp"
#include "usb/memory.hpp"
#include "usb/xhci/registers.hpp"
#include "usb/xhci/trb.hpp"

namespace usb::xhci
{
/** @brief Command/Transfer Ring を表すクラス． */
class Ring
{
public:
  Ring() = default;
  Ring(const Ring &) = delete;
  ~Ring();
  Ring &operator=(const Ring &) = delete;

  /** @brief リングのメモリ領域を割り当て，メンバを初期化する． */
  Error Initialize(size_t buf_size);

  /** @brief TRB に cycle bit を設定した上でリング末尾に追加する．
   *
   * @return 追加された（リング上の）TRB を指すポインタ．
   */
  template <typename TRBType> TRB *Push(const TRBType &trb)
  {
    return Push(trb.data);
  }

  TRB *Buffer() const
  {
    return buf_;
  }

private:
  TRB *buf_ = nullptr;
  size_t buf_size_ = 0;

  /** @brief プロデューサ・サイクル・ステートを表すビット */
  bool cycle_bit_;
  /** @brief リング上で次に書き込む位置 */
  size_t write_index_;

  /** @brief TRB に cycle bit を設定した上でリング末尾に書き込む．
   *
   * write_index_ は変化させない．
   */
  void CopyToLast(const std::array<uint32_t, 4> &data);

  /** @brief TRB に cycle bit を設定した上でリング末尾に追加する．
   *
   * write_index_ をインクリメントする．その結果 write_index_ がリング末尾
   * に達したら LinkTRB を適切に配置して write_index_ を 0 に戻し，
   * cycle bit を反転させる．
   *
   * @return 追加された（リング上の）TRB を指すポインタ．
   */
  TRB *Push(const std::array<uint32_t, 4> &data);
};

union EventRingSegmentTableEntry {
  std::array<uint32_t, 4> data;
  struct
  {
    uint64_t ring_segment_base_address; // 64 バイトアライメント

    uint32_t ring_segment_size : 16;
    uint32_t : 16;

    uint32_t : 32;
  } __attribute__((packed)) bits;
};

class EventRing
{
public:
  Error Initialize(size_t buf_size, InterrupterRegisterSet *interrupter);

  TRB *ReadDequeuePointer() const
  {
    return reinterpret_cast<TRB *>(interrupter_->ERDP.Read().Pointer());
  }

  void WriteDequeuePointer(TRB *p);

  bool HasFront() const
  {
    return Front()->bits.cycle_bit == cycle_bit_;
  }

  TRB *Front() const
  {
    return ReadDequeuePointer();
  }

  void Pop();

private:
  TRB *buf_;
  size_t buf_size_;

  bool cycle_bit_;
  EventRingSegmentTableEntry *erst_;
  InterrupterRegisterSet *interrupter_;
};
} // namespace usb::xhci
