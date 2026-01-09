/**
 * FTL Persistence Tests.
 *
 * Verifies that the FTL can rebuild its state from NAND OOB tags.
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

TEST_CASE("FTL - Persistence Rebuild") {
  NandChip *nand = new NandChip(10);

  // 1. Create FTL and write data
  {
    Ftl ftl(nand, 100);
    std::vector<Byte> data(PageDataSize, 0xCC);

    // Write LBA 5 -> A
    CHECK(ftl.Write(5, data) == NandStatus::Success);

    // Write LBA 10 -> B
    std::vector<Byte> data2(PageDataSize, 0xDD);
    CHECK(ftl.Write(10, data2) == NandStatus::Success);
  }
  // FTL destroyed, but NAND persists!

  // 2. Create NEW FTL (Simulate fresh boot)
  {
    Ftl ftl2(nand, 100);

    // 3. Verify Mapping was restored
    std::vector<Byte> readBack(PageDataSize);

    CHECK(ftl2.Read(5, readBack) == NandStatus::Success);
    CHECK(readBack[0] == 0xCC);

    CHECK(ftl2.Read(10, readBack) == NandStatus::Success);
    CHECK(readBack[0] == 0xDD);
  }

  delete nand;
}
