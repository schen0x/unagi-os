#include "memory_manager.hpp"

BitmapMemoryManager::BitmapMemoryManager() : alloc_map_{}, range_begin_{FrameID{0}}, range_end_{FrameID{kFrameCount}}
{
}

WithError<FrameID> BitmapMemoryManager::Allocate(size_t num_frames)
{
  size_t start_frame_id = range_begin_.ID();
  while (true)
  {
    size_t i = 0;
    for (; i < num_frames; ++i)
    {
      if (start_frame_id + i >= range_end_.ID())
      {
        return {kNullFrame, MAKE_ERROR(Error::kNoEnoughMemory)};
      }
      /* If a frame is occupied */
      if (GetBit(FrameID{start_frame_id + i}))
      {
        break;
      }
    }
    if (i == num_frames)
    {
      // num_frames has been found, return success
      MarkAllocated(FrameID{start_frame_id}, num_frames);
      return {
          FrameID{start_frame_id},
          MAKE_ERROR(Error::kSuccess),
      };
    }
    // do the search again
    start_frame_id += i + 1;
  }
}

/* Free `num_frames` start from `FrameID` by SetBit to false in the Bitmap */
Error BitmapMemoryManager::Free(FrameID start_frame, size_t num_frames)
{
  for (size_t i = 0; i < num_frames; ++i)
  {
    SetBit(FrameID{start_frame.ID() + i}, false);
  }
  return MAKE_ERROR(Error::kSuccess);
}

/* Mark allocated in the memoryManager */
void BitmapMemoryManager::MarkAllocated(FrameID start_frame, size_t num_frames)
{
  for (size_t i = 0; i < num_frames; ++i)
  {
    SetBit(FrameID{start_frame.ID() + i}, true);
  }
}

/* Set the FrameID start and end of the existing memory */
void BitmapMemoryManager::SetMemoryRange(FrameID range_begin, FrameID range_end)
{
  range_begin_ = range_begin;
  range_end_ = range_end;
}

bool BitmapMemoryManager::GetBit(FrameID frame) const
{
  auto line_index = frame.ID() / kBitsPerMapLine;
  auto bit_index = frame.ID() % kBitsPerMapLine;

  return (alloc_map_[line_index] & (static_cast<MapLineType>(1) << bit_index)) != 0;
}

void BitmapMemoryManager::SetBit(FrameID frame, bool allocated)
{
  auto line_index = frame.ID() / kBitsPerMapLine;
  auto bit_index = frame.ID() % kBitsPerMapLine;

  if (allocated)
  {
    alloc_map_[line_index] |= (static_cast<MapLineType>(1) << bit_index);
  }
  else
  {
    alloc_map_[line_index] &= ~(static_cast<MapLineType>(1) << bit_index);
  }
}
