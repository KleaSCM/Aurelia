/**
 * System Integration Tests.
 *
 * Tests the complete system: CPU + Bus + RAM + Loader.
 * Verifies end-to-end program loading and execution.
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#include "Bus/Bus.hpp"
#include "Cpu/Cpu.hpp"
#include "Memory/RamDevice.hpp"
#include "System/Loader.hpp"
#include "System/MemoryMap.hpp"
#include <catch2/catch_test_macros.hpp>
#include <iostream>

using namespace Aurelia;
using namespace Aurelia::System;

TEST_CASE("System - Load and Execute Simple Program") {
  /**
   * INTEGRATION TEST WORKFLOW:
   *
   * 1. Create Bus, RAM, CPU
   * 2. Connect RAM to Bus
   * 3. Use Loader to write program into RAM
   * 4. Reset CPU
   * 5. Execute until HALT
   * 6. Verify register state
   *
   * This tests that all components work together correctly.
   */

  // Setup components
  Bus::Bus bus;
  Memory::RamDevice ram(RamSize, 0); // Zero latency for instant writes
  Cpu::Cpu cpu;

  // Connect to bus
  bus.ConnectDevice(&ram);
  cpu.ConnectBus(&bus);

  // Create simple program: MOV R0, #42; HALT
  // MOV R0, #42 encodes to: 0x80 0x00 0x00 0x2A (little-endian)
  // HALT encodes to: 0xFC 0x00 0x00 0x00
  std::vector<std::uint8_t> program = {
      0x2A, 0x00, 0x00, 0x80, // MOV R0, #42
      0x00, 0x00, 0x00, 0xFC  // HALT
  };

  // Load program into RAM
  Loader loader(bus);
  bool loaded = loader.LoadData(program, ResetVector);
  if (!loaded) {
    std::cerr << "Load failed: " << loader.GetErrorMessage() << "\n";
  }
  REQUIRE(loaded);

  // Reset CPU (PC = 0x0)
  cpu.Reset(ResetVector);
  REQUIRE(cpu.GetPC() == 0x0);

  // Execute until HALT or timeout
  for (int i = 0; i < 50 && !cpu.IsHalted(); ++i) {
    cpu.OnTick();
    bus.OnTick();
  }

  // Verify HALT was reached and R0 = 42
  std::cerr << "Final: Halted=" << cpu.IsHalted() << " PC=0x" << std::hex
            << cpu.GetPC() << std::dec
            << " R0=" << cpu.GetRegister(Cpu::Register::R0) << "\n";
  REQUIRE(cpu.IsHalted());
  REQUIRE(cpu.GetRegister(Cpu::Register::R0) == 42);
}

TEST_CASE("Loader - File Not Found") {
  Bus::Bus bus;
  Loader loader(bus);

  REQUIRE_FALSE(loader.LoadBinary("nonexistent.bin"));
  REQUIRE(!loader.GetErrorMessage().empty());
}

TEST_CASE("Loader - Empty Data") {
  Bus::Bus bus;
  Loader loader(bus);

  std::vector<std::uint8_t> empty;
  REQUIRE_FALSE(loader.LoadData(empty, 0x0));
}

TEST_CASE("Loader - Address Out of Range") {
  Bus::Bus bus;
  Loader loader(bus);

  std::vector<std::uint8_t> data = {0x00, 0x00, 0x00, 0x00};

  // Try to load into MMIO space (should fail)
  REQUIRE_FALSE(loader.LoadData(data, MmioBase));
  REQUIRE(!loader.GetErrorMessage().empty());
}

TEST_CASE("MemoryMap - Address Validation") {
  // RAM addresses
  REQUIRE(IsRamAddress(RamBase));
  REQUIRE(IsRamAddress(RamEnd));
  REQUIRE(IsRamAddress(0x1000));

  // MMIO addresses
  REQUIRE(IsMmioAddress(MmioBase));
  REQUIRE(IsMmioAddress(StorageControllerBase));

  // RAM is not MMIO
  REQUIRE_FALSE(IsMmioAddress(0x1000));

  // MMIO is not RAM
  REQUIRE_FALSE(IsRamAddress(MmioBase));
}
