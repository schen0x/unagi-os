#pragma once

#include <array>
#include <cstddef>

#include "error.hpp"

/**
 * FIFO
 */
template <typename T> class ArrayQueue
{
public:
  template <size_t N> ArrayQueue(std::array<T, N> &buf);
  ArrayQueue(T *buf, size_t size);
  Error Push(const T &value);
  Error Pop();
  size_t Count() const; // const function; a compiler check, fail if write inside the function;
                        // internally, `const ArrayQueue *this`; thus prevent writing on `*this`
  size_t Capacity() const;
  const T &Front() const;
  bool IsEmpty() const;

private:
  T *data_;
  size_t read_pos_, write_pos_, count_;
  /*
   * read_pos_ points to an element to be read.
   * write_pos_ points to a blank position.
   * count_ is the number of elements available.
   */
  const size_t capacity_;
};

/**
 * Constructor 1
 * ArrayQueue(std::array<T, N> &buf)
 */
template <typename T> template <size_t N> ArrayQueue<T>::ArrayQueue(std::array<T, N> &buf) : ArrayQueue(buf.data(), N)
{
}

/**
 * Constructor 2
 * ArrayQueue(T *buf, size_t size)
 */
template <typename T>
ArrayQueue<T>::ArrayQueue(T *buf, size_t size) : data_{buf}, read_pos_{0}, write_pos_{0}, count_{0}, capacity_{size}
{
}

/**
 * enqueue(const T &value); append the reference to the queue in FIFO order
 * return Error::kFull when full
 */
template <typename T> Error ArrayQueue<T>::Push(const T &value)
{
  if (count_ == capacity_)
  {
    return MAKE_ERROR(Error::kFull);
  }

  data_[write_pos_] = value;
  ++count_;
  ++write_pos_;
  if (write_pos_ == capacity_)
  {
    write_pos_ = 0;
  }
  return MAKE_ERROR(Error::kSuccess);
}

/**
 * Trash an element in FIFO order; but does not returns the element
 * Use the Front(); to read that element before removing
 * return Error::kEmpty when empty or Error::kSuccess if success;
 */
template <typename T> Error ArrayQueue<T>::Pop()
{
  if (count_ == 0)
  {
    return MAKE_ERROR(Error::kEmpty);
  }

  --count_;
  ++read_pos_;
  if (read_pos_ == capacity_)
  {
    read_pos_ = 0;
  }
  return MAKE_ERROR(Error::kSuccess);
}

template <typename T> bool ArrayQueue<T>::IsEmpty() const
{
  return count_ == 0;
}

template <typename T> size_t ArrayQueue<T>::Count() const
{
  return count_;
}

template <typename T> size_t ArrayQueue<T>::Capacity() const
{
  return capacity_;
}

/**
 * peek(); reveal an element in FIFO order
 */
template <typename T> const T &ArrayQueue<T>::Front() const
{
  return data_[read_pos_];
}
