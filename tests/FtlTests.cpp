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
  Ftl ftl(&nand, 2);

  std::vector<Byte> data(PageDataSize, 0xFF);

  // Fill Block 0 (Unique LBAs 0-63)
  for (int i = 0; i < 64; ++i)
    CHECK(ftl.Write(i, data) == NandStatus::Success);

  // Fill Block 1 (Unique LBAs 64-127)
  for (int i = 0; i < 64; ++i)
    CHECK(ftl.Write(i + 64, data) == NandStatus::Success);

  // Attempt to write more (Should fail - No Free Blocks & No Garbage to
  // Collect)
  CHECK(ftl.Write(200, data) != NandStatus::Success);
}

TEST_CASE("FTL - GarbageCollection_ReclaimsSpace") {
  // Setup: Small device (4 physical blocks) -> 256 Pages
  // FTL reserves one block for active use, so capacity is ~3 blocks.
  // We initialize FTL with 4 blocks.
  NandChip nand(4);
  Ftl ftl(&nand, 4);

  std::vector<Byte> data(PageDataSize, 0xAA);

  // 1. Fill the device (Fill Blocks 0, 1, 2. Block 3 is active/free).
  // 3 * 64 = 192 pages.
  for (int i = 0; i < 192; ++i) {
    // Write distinct LBAs
    REQUIRE(ftl.Write(i, data) == NandStatus::Success);
  }

  // 2. Device is effectively full (next boundary).
  // Let's invalidate Block 0 (LBA 0..63) by overwriting it.
  // This should make Block 0 a candidate for GC (ValidPageBitmap => 0).
  for (int i = 0; i < 64; ++i) {
    REQUIRE(ftl.Write(i, data) == NandStatus::Success);
  }

  // 3. Force Allocation / GC.
  // We wrote 192 + 64 = 256 pages. Physical capacity is 256.
  // We used all physical pages?
  // Block 0: Invalidated (64 pages used, 0 valid).
  // Block 1: Valid (64 pages).
  // Block 2: Valid (64 pages).
  // Block 3: Valid (64 pages) - This was the overwrite target!

  // So all 4 blocks are physically full/active.
  // Block 0 is "Full" but "Stale".
  // Block 1 is "Full" and "Valid".
  // Block 2 is "Full" and "Valid".
  // Block 3 is "Full" and "Valid" (The new mapping for LBA 0..63).

  // Attempting to write one more page requires a Free Block.
  // The Free List is empty.
  // The Write() should trigger GC -> Reclaim Block 0 -> Use it as new Active.

  REQUIRE(ftl.Write(1000, data) == NandStatus::Success);

  // Verify Block 0 was reused.
  auto info = ftl.GetBlockInfo(0);
  // It should be Active (if picked immediately) or Free (if pushed to list then
  // popped). Since AllocateNewActiveBlock pops the back, and GC pushes to back,
  // it likely picked Block 0.
  bool recycled =
      (info.State == BlockState::Active) || (info.State == BlockState::Free);
  CHECK(recycled);
  CHECK(info.EraseCount == 1); // Verify it was erased
}
