/**
 * CPU State Tests.
 *
 * Verifies Register File access, Reset behavior, and Flag management.
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#include "Cpu/Cpu.hpp"
#include <catch2/catch_test_macros.hpp>

using namespace Aurelia::Cpu;
using namespace Aurelia::Core;

TEST_CASE("Cpu - InitialState") {
  Cpu cpu;
  // Verify all registers 0
  for (int i = 0; i < 16; ++i) {
    CHECK(cpu.GetRegister(static_cast<Register>(i)) == 0);
  }
  CHECK(cpu.GetPC() == 0);

  const Flags &f = cpu.GetFlags();
  CHECK_FALSE(f.Z);
  CHECK_FALSE(f.N);
  CHECK_FALSE(f.C);
  CHECK_FALSE(f.V);
}

TEST_CASE("Cpu - ResetBehavior") {
  Cpu cpu;
  cpu.SetRegister(Register::R0, 0xDEADBEEF);
  cpu.SetPC(0x1000);

  cpu.Reset(0x8000);

  CHECK(cpu.GetRegister(Register::R0) == 0);
  CHECK(cpu.GetPC() == 0x8000);
}

TEST_CASE("Cpu - RegisterAccess") {
  Cpu cpu;
  cpu.SetRegister(Register::R5, 42);
  cpu.SetRegister(Register::SP, 0x100);

  CHECK(cpu.GetRegister(Register::R5) == 42);
  CHECK(cpu.GetRegister(Register::R14) == 0x100); // SP is R14 alias
}
