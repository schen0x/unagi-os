/**
 * @file register.hpp
 *
 * メモリマップトレジスタを読み書きする機能を提供する．
 */

#pragma once

#include <cstddef>
#include <cstdint>

/**
 * Primary template (seems not necessary for now)
 */
template <typename T> struct ArrayLength
{
};

/**
 * Specialized to T[N]
 */
template <typename T, size_t N> struct ArrayLength<T[N]>
{
  static const size_t value = N;
};

/**
 * MemMapRegister is a wrapper for a memory mapped register.
 *
 * MemMapRegister forces user program to read/write the underlying register
 * with specific bit width. The bit width will be deduced from the type of
 * T::data. T is the template parameter. T::data should be an array.
 *
 * Usage:
 * int main() {
 *   MemMapRegister<PORTSC_Bitmap> portsc; // PORTSC_Bitmap is a union
 *   PORTSC_Bitmap p = portsc.Read(); // p.data <-
 *   p.bits.port_reset = true;
 *   portsc.Write(p); // Write the data with a flipped bit
 *   MemMapRegister<DefaultBitmap<uint8_t>> caplength;   // Capability Register Length
 *   DefaultBitmap<uint8_t> c2 = caplength.Read();
 *   c2 = 8;
 *   caplength.Write(c2);
 * }
 */
template <typename T> class MemMapRegister
{
public:
  /* Read the data[] field, and no member data is changed */
  T Read() const
  {
    T tmp;
    for (size_t i = 0; i < len_; ++i)
    {
      tmp.data[i] = value_.data[i];
    }
    return tmp;
  }

  void Write(const T &value)
  {
    for (size_t i = 0; i < len_; ++i)
    {
      value_.data[i] = value.data[i];
    }
  }

private:
  volatile T value_;
  /* decltype(T::data) is replaced into T[1] in compile time, so ArrayLength<T[N]> is used */
  static const size_t len_ = ArrayLength<decltype(T::data)>::value;
};

/**
 * sizeof(DefaultBitmap) == sizeof(T);
 * Note this is a struct not a union
 */
template <typename T> struct DefaultBitmap
{
  T data[1];

  /**
   * Overloadding the assignment operator
   * C++11 Standard Sec. 5.17, the return type is described as "lvalue referring to left hand operand"
   * Usage:
   * int main() {
   *   DefaultBitmap<uint8_t> db;
   *   db=4;
   *   std::cout << (int) db; // 4
   * }
   *
   */
  DefaultBitmap &operator=(const T &value)
  {
    data[0] = value;
    return *this;
  }
  /**
   * operator type; user-defined conversion function;
   * implicit conversion (without "explicit" keyword)
   * e.g.:
   *   struct X { operator int() const { return 7; } }
   *   int main() {
   *     X x;
   *     int n = static_cast<int>(x);   // OK: sets n to 7
   *     int m = x;                     // OK: sets m to 7
   *   }
   */
  operator T() const
  {
    return data[0];
  }
};

/*
 * Design: container-like classes.
 *
 * Container-like classes, such as PortArray and DeviceContextArray,
 * should have Size() method and Iterator type.
 * Size() should return the number of elements, and iterators
 * of that type should iterate all elements.
 *
 * Each element may have a flag indicating availableness of the element.
 * For example each port has "Port Enabled/Disabled" bit.
 * Size() and iterators should not skip disabled elements.
 */

template <typename T> class ArrayWrapper
{
public:
  using ValueType = T;
  using Iterator = ValueType *;
  using ConstIterator = const ValueType *;

  ArrayWrapper(uintptr_t array_base_addr, size_t size)
      : array_(reinterpret_cast<ValueType *>(array_base_addr)), size_(size)
  {
  }

  size_t Size() const
  {
    return size_;
  }

  // begin, end, cbegin, cend must be lower case names
  // to be used in rage-based for statements.
  Iterator begin()
  {
    return array_;
  }
  Iterator end()
  {
    return array_ + size_;
  }
  ConstIterator cbegin() const
  {
    return array_;
  }
  ConstIterator cend() const
  {
    return array_ + size_;
  }

  ValueType &operator[](size_t index)
  {
    return array_[index];
  }

private:
  ValueType *const array_;
  const size_t size_;
};
