/**
 * System Tests.
 *
 * Verifies Clock and System orchestration.
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#include "Core/ITickable.hpp"
#include "Core/System.hpp"
#include <catch2/catch_test_macros.hpp>

using namespace Aurelia::Core;

// Mock tickable device for testing
class MockDevice : public ITickable {
public:
  void OnTick() override { TickCount++; }

  int TickCount = 0;
};

TEST_CASE("System - ClockIncrements") {
  Clock clk;
  CHECK(clk.GetTotalTicks() == 0);
  clk.Tick();
  CHECK(clk.GetTotalTicks() == 1);
}

TEST_CASE("System - SystemRun") {
  System sys;
  MockDevice dev1;
  MockDevice dev2;

  sys.AddDevice(&dev1);
  sys.AddDevice(&dev2);

  sys.Run(10);

  CHECK(sys.GetClock().GetTotalTicks() == 10);
  CHECK(dev1.TickCount == 10);
  CHECK(dev2.TickCount == 10);
}
