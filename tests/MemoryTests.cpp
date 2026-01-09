/**
 * Memory Subsystem Tests.
 *
 * Verifies RAM storage and latency simulation.
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#include "Memory/RamDevice.hpp"
#include <catch2/catch_test_macros.hpp>

using namespace Aurelia;
using namespace Aurelia::Memory;
using namespace Aurelia::Core;

TEST_CASE("Memory - StorageAccess") {
  // Zero latency for simple storage test
  RamDevice ram(1024, 0);
  ram.SetBaseAddress(0x1000);

  Data writeVal = 0xAA55AA55;
  Data readVal = 0;

  // Write
  CHECK(ram.OnWrite(0x1000, writeVal));

  // Read back
  CHECK(ram.OnRead(0x1000, readVal));
  CHECK(readVal == writeVal);
}

TEST_CASE("Memory - LatencySimulation") {
  // 2 Ticks latency
  RamDevice ram(1024, 2);
  ram.SetBaseAddress(0x1000);

  Data writeVal = 0xBEEF;

  // Tick 0: Request Write -> Should NOT complete
  CHECK_FALSE(ram.OnWrite(0x1000, writeVal));

  // Tick 1: Still waiting
  ram.OnTick();
  CHECK_FALSE(ram.OnWrite(0x1000, writeVal));

  // Tick 2: Must override wait count?
  // The device decrements logic relies on consecutive checking or bus holding
  // state. In our simple test:
  ram.OnTick(); // WaitTicks = 1 -> 0

  // Tick 2: Should be ready now!
  CHECK(ram.OnWrite(0x1000, writeVal));
}
