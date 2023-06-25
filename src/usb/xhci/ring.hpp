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
 *
 * 4.9.2 Transfer Ring Management
 *   - Software uses and maintains private copies of the Enqueue and Dequeue Pointers for each Transfer Ring.
 *   - Both are initialized to the address of the first TRB location in the Transfer Ring
 *   - and written to the Endpoint/Stream Context Transfer Ring Dequeue Pointer field
 *
 * 4.9.4 Event Ring Management
 *
 * The xHC maintains an Event Ring Producer Cycle State (PCS) bit, initializing it to ‘1’ and toggling it every time the
 * Event Ring Enqueue Pointer wraps back to the beginning of the Event Ring. The value of the PCS bit is written to the
 * Cycle bit when the xHC generates an Event TRB on the Event Ring.
 *
 * Software maintains an Event Ring Consumer Cycle State (CCS) bit, initializing it to ‘1’ and toggling it every time
 * the Event Ring Dequeue Pointer wraps back to the beginning of the Event Ring.
 *
 * Figure 4-6: Index Management
 * Enqueue Pointer: managed by the producer; producer maintains a Producer
 *   Cycle State (PCS) flag which identifies the value that it shall write to the
 *   TRB Cycle bit.
 * Dequeue Pointer: managed by the consumer; consumer maintains a Consumer
 *   Cycle State (CCS) flag, which it compares to the Cycle bit in TRBs that it
 *   fetches.
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

  /**
   * The Producer Cycle State (PCS) bit;
   * (Set during "enqueue", hence by the Producer -- for Command/Transfer Ring, by the Host)
   *
   * - A ring may have multiple segments
   * - The PCS is initialized to '1'
   * - The PCS is used when enqueuing, written to each TRB
   * - The PCS is flipped **only** when the Enqueue Pointer wraps back to the beginning of the Ring
   *
   * Figure 4-10: Final State of Transfer Ring
   */
  bool cycle_bit_;
  /**
   * TRB *enqueuePtr = &buf_[write_index_];
   */
  size_t write_index_;

  /**
   * Write data at enqueuePtr; does not increment the pointer
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

/**
 * 4.9.4 Event Ring Management
 * EventRing vs Transfer/Command Ring, a fundamental difference is that:
 * - the xHC is the producer, and system software is the consumer of Event TRBs.
 *
 * Figure 4-12: Event Ring State Machine
 * Figure 4-11: Segmented Event Ring Example
 *
 * Event Ring segments are defined by an Event Ring Segment Table (ERST).
 * The ERST consists of an array of Base Address/Size pairs (ERST.BaseAddress
 * and ERST.Size), each defining a single Event Ring segmen t. The first
 * element in the ERST (0) is pointed to by the ERST Base Address Register
 * (ERSTBA section 5.5.2.3.2). The number of elements in the ERST is defined by
 * the ERST Size Register (ERSTSZ section 5.5.2.3.1).
 */
class EventRing
{
public:
  Error Initialize(size_t buf_size, InterrupterRegisterSet *interrupter);

  /**
   * TRB *getDequeuePointer()
   *
   * Read the Event Ring Dequeue Pointer (ERDP) from the Event Ring Registers,
   * which reside in the Runtime Register Space.
   */
  TRB *ReadDequeuePointer() const
  {
    return reinterpret_cast<TRB *>(interrupter_->ERDP.Read().Pointer());
  }

  /**
   * The Event Ring Dequeue Pointer Register (ERDP) is written by **software** to
   * define the Event Ring Dequeue Pointer location to the xHC. Software
   * updates this pointer when it is finished the evaluation of an Event(s) on
   * the Event Ring.
   */
  void WriteDequeuePointer(TRB *p);

  /**
   * If (EventTRB.cycle_bit == CSS);
   * If false, ring is empty, WriteDequeuePointer(TRB *thisTRB);
   */
  bool HasFront() const
  {
    return Front()->bits.cycle_bit == cycle_bit_;
  }

  /**
   * TRB *getDequeuePointer()
   */
  TRB *Front() const
  {
    return ReadDequeuePointer();
  }

  void Pop();

private:
  TRB *buf_;
  size_t buf_size_;

  /**
   * The Event Ring Consumer Cycle State (CCS) bit;
   * - the xHC maintains an Event Ring Producer Cycle State (PCS) bit; initialized to '1'
   * - the software maintains an Event Ring Consumer Cycle State (CCS) bit;
   *   initializing it to ‘1’; toggle every time the Dequeue Pointer wraps back to the head.
   * - If EventTRB.cycle_bit == CSS, then the EventTRB is valid event, software processes it and advances
   *   the Dequeue Pointer.
   * - Otherwise, stops processing Event TRBs and waits for an interrupt
   *   from the xHC for the Event Ring.
   * - When the interrupt occurs, software picks up where it left off and compare the bits again.
   *
   * - If EventTRB.cycle_bit != CSS, the Event Ring is empty, write the address
   *   of this TRB to Event Ring Dequeue Pointer Register (ERDP) to indicate
   *   that it has processed all Events in the ring.
   */
  bool cycle_bit_;
  EventRingSegmentTableEntry *erst_;
  InterrupterRegisterSet *interrupter_;
};
} // namespace usb::xhci
