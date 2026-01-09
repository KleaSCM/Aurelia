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
#include <gtest/gtest.h>

using namespace Aurelia::Core;

TEST(BitManipTest, SetBit) {
  Byte val = 0;
  val = SetBit(val, 2);
  EXPECT_EQ(val, 4);
}

TEST(BitManipTest, CheckBit) {
  Byte val = 0b00001010;
  EXPECT_TRUE(CheckBit(val, 1));
  EXPECT_FALSE(CheckBit(val, 0));
  EXPECT_TRUE(CheckBit(val, 3));
}

TEST(BitManipTest, ExtractBits) {
  // Binary: 1100 1010
  // Hex: 0xCA
  Byte val = 0xCA;

  // Extract bits 4-7 (1100) -> 12 (0xC)
  EXPECT_EQ(ExtractBits(val, 4, 4), 0xC);

  // Extract bits 0-3 (1010) -> 10 (0xA)
  EXPECT_EQ(ExtractBits(val, 0, 4), 0xA);
}
