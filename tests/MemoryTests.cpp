/**
 * Memory Subsystem Tests.
 *
 * Verifies RAM storage and latency simulation.
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#include "Memory/RamDevice.hpp"
#include <gtest/gtest.h>

using namespace Aurelia;
using namespace Aurelia::Memory;
using namespace Aurelia::Core;

TEST(MemoryTest, StorageAccess) {
  // Zero latency for simple storage test
  RamDevice ram(1024, 0);
  ram.SetBaseAddress(0x1000);

  Data writeVal = 0xAA55AA55;
  Data readVal = 0;

  // Write
  EXPECT_TRUE(ram.OnWrite(0x1000, writeVal));

  // Read back
  EXPECT_TRUE(ram.OnRead(0x1000, readVal));
  EXPECT_EQ(readVal, writeVal);
}

TEST(MemoryTest, LatencySimulation) {
  // 2 Ticks latency
  RamDevice ram(1024, 2);
  ram.SetBaseAddress(0x1000);

  Data writeVal = 0xBEEF;

  // Tick 0: Request Write -> Should NOT complete
  EXPECT_FALSE(ram.OnWrite(0x1000, writeVal));

  // Tick 1: Still waiting
  ram.OnTick();
  EXPECT_FALSE(ram.OnWrite(0x1000, writeVal));

  // Tick 2: Must override wait count?
  // The device decrements logic relies on consecutive checking or bus holding
  // state. In our simple test:
  ram.OnTick(); // WaitTicks = 1 -> 0

  // Tick 2: Should be ready now!
  EXPECT_TRUE(ram.OnWrite(0x1000, writeVal));
}
