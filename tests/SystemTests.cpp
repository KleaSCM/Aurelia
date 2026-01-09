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
#include <gtest/gtest.h>

using namespace Aurelia::Core;

// Mock tickable device for testing
class MockDevice : public ITickable {
public:
  void OnTick() override { TickCount++; }

  int TickCount = 0;
};

TEST(SystemTest, ClockIncrements) {
  Clock clk;
  EXPECT_EQ(clk.GetTotalTicks(), 0);
  clk.Tick();
  EXPECT_EQ(clk.GetTotalTicks(), 1);
}

TEST(SystemTest, SystemRun) {
  System sys;
  MockDevice dev1;
  MockDevice dev2;

  sys.AddDevice(&dev1);
  sys.AddDevice(&dev2);

  sys.Run(10);

  EXPECT_EQ(sys.GetClock().GetTotalTicks(), 10);
  EXPECT_EQ(dev1.TickCount, 10);
  EXPECT_EQ(dev2.TickCount, 10);
}
