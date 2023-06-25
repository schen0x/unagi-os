#include "usb/xhci/ring.hpp"

#include "usb/memory.hpp"

namespace usb::xhci
{
Ring::~Ring()
{
  if (buf_ != nullptr)
  {
    FreeMem(buf_);
  }
}

Error Ring::Initialize(size_t buf_size)
{
  if (buf_ != nullptr)
  {
    FreeMem(buf_);
  }

  /**
   * Sync with the xHC hardware
   * - the xHC maintains an Event Ring Producer Cycle State (PCS) bit; initialized to '1'
   */
  cycle_bit_ = true;
  write_index_ = 0;
  buf_size_ = buf_size;

  buf_ = AllocArray<TRB>(buf_size_, 64, 64 * 1024);
  if (buf_ == nullptr)
  {
    return MAKE_ERROR(Error::kNoEnoughMemory);
  }
  memset(buf_, 0, buf_size_ * sizeof(TRB));

  return MAKE_ERROR(Error::kSuccess);
}

/**
 * Write data at enqueuePtr
 * cycle_bit not included
 */
void Ring::CopyToLast(const std::array<uint32_t, 4> &data)
{
  TRB *enqueuePtr = &buf_[write_index_];
  for (int i = 0; i < 3; ++i)
  {
    // data[0..2] must be written prior to data[3].
    enqueuePtr->data[i] = data[i];
  }
  enqueuePtr->data[3] = data[3];
  enqueuePtr->bits.cycle_bit = cycle_bit_;
}

/**
 * Automatically set the cycle_bit_ according to the enqueuePtr,
 * because in Command and Transfer ring, the Host is the Producer of the &data
 */
TRB *Ring::Push(const std::array<uint32_t, 4> &data)
{
  TRB *enqueuePtr = &buf_[write_index_];
  CopyToLast(data);

  ++write_index_;
  /**
   * Add another segment (actually, wrap to the head of the ring itself)
   */
  if (write_index_ == buf_size_ - 1)
  {
    LinkTRB link{buf_};            // next segment = (TRB *) buf_; trb_type = linkTRB;
    link.bits.toggle_cycle = true; // TC = 1; because the ring is wrapped
    CopyToLast(link.data);         // Write

    write_index_ = 0;
    cycle_bit_ = !cycle_bit_;
  }

  return enqueuePtr;
}

Error EventRing::Initialize(size_t buf_size, InterrupterRegisterSet *interrupter)
{
  if (buf_ != nullptr)
  {
    FreeMem(buf_);
  }

  cycle_bit_ = true;
  buf_size_ = buf_size;
  interrupter_ = interrupter;

  buf_ = AllocArray<TRB>(buf_size_, 64, 64 * 1024);
  if (buf_ == nullptr)
  {
    return MAKE_ERROR(Error::kNoEnoughMemory);
  }
  memset(buf_, 0, buf_size_ * sizeof(TRB));

  erst_ = AllocArray<EventRingSegmentTableEntry>(1, 64, 64 * 1024);
  if (erst_ == nullptr)
  {
    FreeMem(buf_);
    return MAKE_ERROR(Error::kNoEnoughMemory);
  }
  memset(erst_, 0, 1 * sizeof(EventRingSegmentTableEntry));

  erst_[0].bits.ring_segment_base_address = reinterpret_cast<uint64_t>(buf_);
  erst_[0].bits.ring_segment_size = buf_size_;

  ERSTSZ_Bitmap erstsz = interrupter_->ERSTSZ.Read();
  erstsz.SetSize(1);
  interrupter_->ERSTSZ.Write(erstsz);

  WriteDequeuePointer(&buf_[0]);

  ERSTBA_Bitmap erstba = interrupter_->ERSTBA.Read();
  erstba.SetPointer(reinterpret_cast<uint64_t>(erst_));
  interrupter_->ERSTBA.Write(erstba);

  return MAKE_ERROR(Error::kSuccess);
}

void EventRing::WriteDequeuePointer(TRB *p)
{
  auto erdp = interrupter_->ERDP.Read();
  erdp.SetPointer(reinterpret_cast<uint64_t>(p));
  interrupter_->ERDP.Write(erdp);
}

/**
 * increment the dequeuePointer
 * if wrap, flip the Event Ring Consumer Cycle State (CCS) bit
 * When software finishes processing an Event TRB, it will write the address of that Event TRB to the ERDP.
 */
void EventRing::Pop()
{
  TRB *p = ReadDequeuePointer() + 1;

  TRB *segment_begin = reinterpret_cast<TRB *>(erst_[0].bits.ring_segment_base_address);
  TRB *segment_end = segment_begin + erst_[0].bits.ring_segment_size;

  if (p == segment_end)
  {
    p = segment_begin;
    cycle_bit_ = !cycle_bit_;
  }

  /* ERSTSZ == 1, no need to update the ERST Segment Index (DESI) field */
  WriteDequeuePointer(p);
}
} // namespace usb::xhci
