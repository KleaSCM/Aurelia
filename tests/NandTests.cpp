/**
 * NAND Physics Tests.
 *
 * Verifies Program/Erase constraints and bitwise operations.
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#include "Storage/Nand/NandChip.hpp"
#include <gtest/gtest.h>
#include <vector>

using namespace Aurelia::Storage::Nand;
using namespace Aurelia::Core;

TEST(NandTest, InitialStateIsErased) {
  // 10 Blocks
  NandChip chip(10);
  std::vector<Byte> buffer(PageDataSize);

  // Read Block 0, Page 0
  NandStatus status = chip.ReadPage(0, 0, buffer);
  EXPECT_EQ(status, NandStatus::Success);

  // Verify all bytes are 0xFF
  for (auto b : buffer) {
    EXPECT_EQ(b, 0xFF);
  }
}

TEST(NandTest, ProgramSuccess) {
  NandChip chip(1);
  std::vector<Byte> data(PageDataSize, 0x00); // Program to 00s
  std::vector<Byte> readBack(PageDataSize);

  // NOTE (KleaSCM) Writing 0x00 to 0xFF is valid (1->0 transition).
  EXPECT_EQ(chip.ProgramPage(0, 0, data), NandStatus::Success);

  EXPECT_EQ(chip.ReadPage(0, 0, readBack), NandStatus::Success);
  EXPECT_EQ(readBack[0], 0x00);
}

TEST(NandTest, ProgramFailure_BitFlipConstraint) {
  NandChip chip(1);
  std::vector<Byte> zeros(PageDataSize, 0x00);
  std::vector<Byte> ones(PageDataSize, 0xFF);

  // 1. Program to 0x00 (Valid)
  EXPECT_EQ(chip.ProgramPage(0, 0, zeros), NandStatus::Success);

  // 2. Try to Program back to 0xFF (Invalid 0->1 transition without erase)
  NandStatus status = chip.ProgramPage(0, 0, ones);

  // NOTE (KleaSCM) This must fail! Flash cells cannot turn 0 back to 1 via
  // programming.
  EXPECT_EQ(status, NandStatus::WriteError);
}

TEST(NandTest, EraseRecover) {
  NandChip chip(1);
  std::vector<Byte> zeros(PageDataSize, 0x00);
  std::vector<Byte> readBack(PageDataSize);

  // 1. Program to 0x00
  EXPECT_EQ(chip.ProgramPage(0, 0, zeros), NandStatus::Success);

  // 2. Erase Block
  EXPECT_EQ(chip.EraseBlock(0), NandStatus::Success);

  // 3. Verify it is back to 0xFF
  EXPECT_EQ(chip.ReadPage(0, 0, readBack), NandStatus::Success);
  EXPECT_EQ(readBack[0], 0xFF);
}
