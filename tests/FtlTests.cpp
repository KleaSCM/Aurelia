/**
 * FTL Strict Tests.
 *
 * Verifies strict Block Management, Allocation, and State consistency.
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#include "Storage/FTL/Ftl.hpp"
#include <catch2/catch_test_macros.hpp>
#include <vector>

using namespace Aurelia::Storage::FTL;
using namespace Aurelia::Storage::Nand;
using namespace Aurelia::Core;

TEST_CASE("FTL - BlockTransitionAndAllocation") {
  // NOTE (KleaSCM) Initialize NAND with 5 physical blocks.
  NandChip nand(5);
  Ftl ftl(&nand, 100);

  std::vector<Byte> data(PageDataSize, 0xAA);

  // 1. Verify Block 0 is Active
  CHECK(ftl.GetBlockInfo(0).State == BlockState::Active);

  // 2. Fill Block 0 (64 pages)
  for (int i = 0; i < 64; ++i) {
    CHECK(ftl.Write(0, data) ==
          NandStatus::Success); // Write to LBA 0 repeatedly
  }

  // 3. Verify Block 0 is Full
  // The NEXT write should trigger the transition to Block 1.
  CHECK(ftl.Write(0, data) == NandStatus::Success);

  CHECK(ftl.GetBlockInfo(0).State == BlockState::Full);
  CHECK(ftl.GetBlockInfo(1).State == BlockState::Active);
}

TEST_CASE("FTL - DeviceFullBehavior") {
  // NOTE (KleaSCM) Initialize small NAND with 2 physical blocks.
  NandChip nand(2);
  Ftl ftl(&nand, 100);

  std::vector<Byte> data(PageDataSize, 0xFF);

  // Fill Block 0
  for (int i = 0; i < 64; ++i)
    CHECK(ftl.Write(0, data) == NandStatus::Success);

  // Fill Block 1
  for (int i = 0; i < 64; ++i)
    CHECK(ftl.Write(0, data) == NandStatus::Success);

  // Attempt to write more (Should fail - No Free Blocks)
  CHECK(ftl.Write(0, data) != NandStatus::Success);
}
