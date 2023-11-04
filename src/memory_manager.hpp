/**
 * @file memory_manager.hpp
 *
 * Memory Management
 */

#pragma once

#include <array>
#include <limits>

#include "error.hpp"

namespace
{
/* User defined literal */
constexpr unsigned long long operator""_KiB(unsigned long long kib)
{
  return kib * 1024;
}

constexpr unsigned long long operator""_MiB(unsigned long long mib)
{
  return mib * 1024_KiB;
}

constexpr unsigned long long operator""_GiB(unsigned long long gib)
{
  return gib * 1024_MiB;
}
} // namespace

/** @brief minimum physical memory frame size */
static const auto kBytesPerFrame{4_KiB};

class FrameID
{
public:
  explicit FrameID(size_t id) : id_{id}
  {
  }
  size_t ID() const
  {
    return id_;
  }
  void *Frame() const
  {
    return reinterpret_cast<void *>(id_ * kBytesPerFrame);
  }

private:
  size_t id_;
};

/* a FrameID object representing "no such frame" */
static const FrameID kNullFrame{std::numeric_limits<size_t>::max()};

/** @brief Memory Manager; implementated in Bitmap
 *
 * Each bit in the alloc_map_ represents a frame;
 * 0 for empty, 1 for allocated;
 *
 * FrameID represented by bit m of the alloc_map_[n] is:
 *   n * bitsPerMapLine + m
 *   @physicaladdress: FrameID * frameBytes == (n * bitsPerMapLine + m) * 4KB
 */
class BitmapMemoryManager
{
public:
  /** @brief Max bytes that this memory management class can handle */
  static const auto kMaxPhysicalMemoryBytes{128_GiB}; // 16 GB -> 0.5MB BitMap
  /** @brief Total frames necessary if kMaxPhysicalMemoryBytes needs to be managed */
  static const auto kFrameCount{kMaxPhysicalMemoryBytes / kBytesPerFrame};

  /** @brief Type of the elm in the alloc_map_  */
  using MapLineType = unsigned long;
  /** @brief sizeof(MapLineType) * 8 is the total bits == number of the frames representable by a MapLine */
  static const size_t kBitsPerMapLine{8 * sizeof(MapLineType)};

  BitmapMemoryManager();

  /** @brief allocate the frames and return the first frameID */
  WithError<FrameID> Allocate(size_t num_frames);
  Error Free(FrameID start_frame, size_t num_frames);
  void MarkAllocated(FrameID start_frame, size_t num_frames);

  /** @brief Set the existing memory range on class init
   * Allocate is suppose to be performed only within range
   *
   * @param range_begin_ Set the FrameID start of the existing memory
   * @param range_end_   Set the FrameID end of the existing memory (exclusive, the range_end_ frame does not exist)
   */
  void SetMemoryRange(FrameID range_begin, FrameID range_end);

private:
  std::array<MapLineType, kFrameCount / kBitsPerMapLine> alloc_map_;
  /** @brief the Frame ID where the existing memory starts */
  FrameID range_begin_;
  /** @brief the Frame ID where the existing memory ends */
  FrameID range_end_;

  bool GetBit(FrameID frame) const;
  void SetBit(FrameID frame, bool allocated);
};
Error InitializeHeap(BitmapMemoryManager &memory_manager);
