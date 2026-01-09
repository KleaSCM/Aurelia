/**
 * Core Types Test.
 *
 * Verifies Bit Manipulation logic.
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#include "Core/BitManip.hpp"
#include "Core/Types.hpp"
#include <catch2/catch_test_macros.hpp>

using namespace Aurelia::Core;

TEST_CASE("BitManip - SetBit") {
  Byte val = 0;
  val = SetBit(val, 2);
  CHECK(val == 4);
}

TEST_CASE("BitManip - CheckBit") {
  Byte val = 0b00001010;
  CHECK(CheckBit(val, 1));
  CHECK_FALSE(CheckBit(val, 0));
  CHECK(CheckBit(val, 3));
}

TEST_CASE("BitManip - ExtractBits") {
  // Binary: 1100 1010
  // Hex: 0xCA
  Byte val = 0xCA;

  // Extract bits 4-7 (1100) -> 12 (0xC)
  CHECK(ExtractBits(val, 4, 4) == 0xC);

  // Extract bits 0-3 (1010) -> 10 (0xA)
  CHECK(ExtractBits(val, 0, 4) == 0xA);
}
