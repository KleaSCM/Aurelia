/**
 * Bit Manipulation Utilities.
 *
 * Provides safe, template-based bitwise operations for handling
 * registers, flags, and bus signals.
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#pragma once

#include <concepts>

namespace Aurelia::Core {

/**
 * Sets the N-th bit of a value to 1.
 */
template <typename T>
  requires std::integral<T>
[[nodiscard]] constexpr T SetBit(T value, std::size_t bitIndex) {
  return static_cast<T>(value | (static_cast<T>(1) << bitIndex));
}

/**
 * Clears the N-th bit of a value (sets to 0).
 */
template <typename T>
  requires std::integral<T>
[[nodiscard]] constexpr T ClearBit(T value, std::size_t bitIndex) {
  return static_cast<T>(value & ~(static_cast<T>(1) << bitIndex));
}

/**
 * Toggles the N-th bit of a value.
 */
template <typename T>
  requires std::integral<T>
[[nodiscard]] constexpr T ToggleBit(T value, std::size_t bitIndex) {
  return static_cast<T>(value ^ (static_cast<T>(1) << bitIndex));
}

/**
 * Checks if the N-th bit is set to 1.
 */
template <typename T>
  requires std::integral<T>
[[nodiscard]] constexpr bool CheckBit(T value, std::size_t bitIndex) {
  return (value & (static_cast<T>(1) << bitIndex)) != 0;
}

/**
 * Extracts a range of bits from the value.
 *
 * @param value The source value.
 * @param start The starting bit index (LSB).
 * @param length The number of bits to extract.
 * @return The extracted value, right-aligned.
 */
template <typename T>
  requires std::integral<T>
[[nodiscard]] constexpr T ExtractBits(T value, std::size_t start,
                                      std::size_t length) {
  // Use uint64_t for calculation to prevent overflow/promotion issues, then
  // cast back
  if (length == 0)
    return 0;
  if (length >= sizeof(T) * 8)
    return value >> start;

  const auto calcMask = (1ULL << length) - 1;
  const T mask = static_cast<T>(calcMask);
  return (value >> start) & mask;
}

} // namespace Aurelia::Core
