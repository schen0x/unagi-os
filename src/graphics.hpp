#pragma once

// #@@range_begin(pixel_color_def)
#include "frame_buffer_config.hpp"

struct PixelColor
{
  uint8_t r, g, b;
};
// #@@range_end(pixel_color_def)

// #@@range_begin(pixel_writer)
class PixelWriter
{
public:
  PixelWriter(const FrameBufferConfig &config) : config_{config}
  {
  }
  virtual ~PixelWriter() = default;
  virtual void Write(int x, int y, const PixelColor &c) = 0;

protected:
  uint8_t *PixelAt(int x, int y)
  {
    return (uint8_t *)config_.frame_buffer_base + 4 * (config_.pixels_per_scan_line * y + x);
  }

private:
  const FrameBufferConfig &config_;
};
// #@@range_end(pixel_writer)

// #@@range_begin(pixel_writer_def)
class RGBResv8BitPerColorPixelWriter : public PixelWriter
{
public:
  using PixelWriter::PixelWriter;
  virtual void Write(int x, int y, const PixelColor &c) override;
};

class BGRResv8BitPerColorPixelWriter : public PixelWriter
{
public:
  using PixelWriter::PixelWriter;
  virtual void Write(int x, int y, const PixelColor &c) override;
};
// #@@range_end(pixel_writer_def)

// #@@range_begin(vector2d)
template <typename T> struct Vector2D
{
  T x, y;
    template <typename U>
  Vector2D<T>& operator +=(const Vector2D<U>& rhs) {
    x += rhs.x;
    y += rhs.y;
    return *this;
  }
};
// #@@range_end(vector2d)

void DrawRectangle(PixelWriter &writer, const Vector2D<int> &pos, const Vector2D<int> &size, const PixelColor &c);

void FillRectangle(PixelWriter &writer, const Vector2D<int> &pos, const Vector2D<int> &size, const PixelColor &c);
