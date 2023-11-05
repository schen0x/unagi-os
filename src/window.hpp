/**
 * @file window.hpp
 *
 * The Window class which represents the "display" area
 */

#pragma once

#include "graphics.hpp"
#include <optional>
#include <vector>

/**
 * The "display" area, (including the cursor)
 */
class Window
{
public:
  /** The PixelWriter for the Window Class
   */
  class WindowWriter : public PixelWriter
  {
  public:
    WindowWriter(Window &window) : window_{window}
    {
    }
    /** Write PixelColor at (x,y) */
    virtual void Write(int x, int y, const PixelColor &c) override
    {
      window_.At(x, y) = c;
    }
    /** @brief Width of the Window, in pixels */
    virtual int Width() const override
    {
      return window_.Width();
    }
    /** @brief Height of the Window, in pixels */
    virtual int Height() const override
    {
      return window_.Height();
    }

  private:
    Window &window_;
  };

  /** Create a Window of width x height pixels */
  Window(int width, int height);
  ~Window() = default;
  Window(const Window &rhs) = delete;
  Window &operator=(const Window &rhs) = delete;

  /**
   * DrawTo the window PixelWriter is bind to
   *
   * @param writer
   * @param position The left top as (0, 0)
   */
  void DrawTo(PixelWriter &writer, Vector2D<int> position);
  void SetTransparentColor(std::optional<PixelColor> c);
  /** Get the WindowWriter of this Window class  */
  WindowWriter *Writer();

  /** Get the PixelColor at (x, y) */
  PixelColor &At(int x, int y);
  /** Get the PixelColor at (x, y) */
  const PixelColor &At(int x, int y) const;

  /** Get the Width in pixel */
  int Width() const;
  /** Get the Height in pixel */
  int Height() const;

private:
  int width_, height_;
  /* data_[y][x] */
  std::vector<std::vector<PixelColor>> data_{};
  WindowWriter writer_{*this};
  std::optional<PixelColor> transparent_color_{std::nullopt};
};
