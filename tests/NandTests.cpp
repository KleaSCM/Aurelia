/**
 * NAND Physics Tests.
 *
 * Verifies Program/Erase constraints and bitwise operations.
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#include "Storage/Nand/NandChip.hpp"
#include <catch2/catch_test_macros.hpp>

using namespace Aurelia::Storage::Nand;
using namespace Aurelia::Core;

TEST_CASE("Nand - InitialStateIsErased") {
  // 10 Blocks
  NandChip chip(10);
  std::vector<Byte> buffer(PageDataSize);

  // Read Block 0, Page 0
  NandStatus status = chip.ReadPage(0, 0, buffer);
  CHECK(status == NandStatus::Success);

  // Verify all bytes are 0xFF
  for (auto b : buffer) {
    CHECK(b == 0xFF);
  }
}

TEST_CASE("Nand - ProgramSuccess") {
  NandChip chip(1);
  std::vector<Byte> data(PageDataSize, 0x00); // Program to 00s
  std::vector<Byte> readBack(PageDataSize);

  // NOTE (KleaSCM) Writing 0x00 to 0xFF is valid (1->0 transition).
  CHECK(chip.ProgramPage(0, 0, data) == NandStatus::Success);

  CHECK(chip.ReadPage(0, 0, readBack) == NandStatus::Success);
  CHECK(readBack[0] == 0x00);
}

TEST_CASE("Nand - ProgramFailure_BitFlipConstraint") {
  NandChip chip(1);
  std::vector<Byte> zeros(PageDataSize, 0x00);
  std::vector<Byte> ones(PageDataSize, 0xFF);

  // 1. Program to 0x00 (Valid)
  CHECK(chip.ProgramPage(0, 0, zeros) == NandStatus::Success);

  // 2. Try to Program back to 0xFF (Invalid 0->1 transition without erase)
  NandStatus status = chip.ProgramPage(0, 0, ones);

  // NOTE (KleaSCM) This must fail! Flash cells cannot turn 0 back to 1 via
  // programming.
  CHECK(status == NandStatus::WriteError);
}

TEST_CASE("Nand - EraseRecover") {
  NandChip chip(1);
  std::vector<Byte> zeros(PageDataSize, 0x00);
  std::vector<Byte> readBack(PageDataSize);

  // 1. Program to 0x00
  CHECK(chip.ProgramPage(0, 0, zeros) == NandStatus::Success);

  // 2. Erase Block
  CHECK(chip.EraseBlock(0) == NandStatus::Success);

  // 3. Verify it is back to 0xFF
  CHECK(chip.ReadPage(0, 0, readBack) == NandStatus::Success);
  CHECK(readBack[0] == 0xFF);
}
