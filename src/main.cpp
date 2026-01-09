/**
 * Aurelia Virtual Machine Entry Point.
 *
 * Complete system integration: CPU + Bus + RAM + Storage + Loader.
 * Loads assembled binaries and executes them on the simulated CPU.
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#include "Bus/Bus.hpp"
#include "Cpu/Cpu.hpp"
#include "Memory/RamDevice.hpp"
#include "Storage/Controller/StorageController.hpp"
#include "System/Loader.hpp"
#include "System/MemoryMap.hpp"
#include <iostream>
#include <string>

using namespace Aurelia;

/**
 * @brief Prints system banner and configuration.
 */
void PrintBanner() {
  std::cout << "╔════════════════════════════════════════════╗\n"
            << "║     Aurelia Virtual System v0.1.0          ║\n"
            << "║     Modern CPU Emulator & Assembler        ║\n"
            << "╚════════════════════════════════════════════╝\n"
            << "\n";
}

/**
 * @brief Prints memory map and system configuration.
 */
void PrintConfiguration() {
  std::cout << "System Configuration:\n"
            << "  Architecture: 64-bit RISC-like\n"
            << "  ISA: Aurelia v2.0 (32 registers)\n"
            << "  Instruction Width: 32-bit fixed\n"
            << "  Endianness: Little-endian\n"
            << "\n";

  std::cout << "Memory Map:\n"
            << "  0x00000000 - 0x0FFFFFFF  RAM (256 MB)\n"
            << "  0xE0000000 - 0xE0000FFF  Storage Controller MMIO\n"
            << "\n";

  std::cout << "Reset Vector: 0x" << std::hex << System::ResetVector << std::dec
            << "\n"
            << "Stack Pointer: 0x" << std::hex << System::InitialStackPointer
            << std::dec << "\n"
            << "\n";
}

/**
 * @brief Main system integration and execution loop.
 *
 * WORKFLOW:
 * 1. Initialize all hardware components
 * 2. Connect devices to Bus
 * 3. Load program binary (if provided)
 * 4. Reset CPU
 * 5. Execute program
 * 6. Report final state
 */
int main(int argc, char *argv[]) {
  PrintBanner();
  PrintConfiguration();

  /**
   * COMPONENT INSTANTIATION
   *
   * Create all hardware components. Order doesn't matter here,
   * they're just allocated on the stack.
   */
  std::cout << "Initializing hardware...\n";

  Bus::Bus bus;
  Memory::RamDevice ram(System::RamSize, 0); // Zero latency
  Cpu::Cpu cpu;

  // Storage controller (optional, for future use)
  // Storage::Controller::StorageController storage(...);

  std::cout << "  [✓] Bus initialized\n";
  std::cout << "  [✓] RAM initialized (" << (System::RamSize / (1024 * 1024))
            << " MB)\n";
  std::cout << "  [✓] CPU initialized\n";
  std::cout << "\n";

  /**
   * BUS WIRING
   *
   * Connect devices to the Bus. The Bus uses IBusDevice::IsAddressInRange()
   * to route read/write requests to the correct device.
   */
  std::cout << "Connecting devices to bus...\n";

  bus.ConnectDevice(&ram);
  cpu.ConnectBus(&bus);
  // bus.ConnectDevice(&storage); // Future

  std::cout << "  [✓] RAM connected to bus\n";
  std::cout << "  [✓] CPU connected to bus\n";
  std::cout << "\n";

  /**
   * PROGRAM LOADING
   *
   * If a binary file is provided as argv[1], load it into RAM at address 0x0.
   * This simulates a bootloader loading a program from disk into memory.
   */
  if (argc > 1) {
    std::string programFile = argv[1];
    std::cout << "Loading program: " << programFile << "\n";

    System::Loader loader(bus);
    if (!loader.LoadBinary(programFile, System::ResetVector)) {
      std::cerr << "❌ Load failed: " << loader.GetErrorMessage() << "\n";
      return 2;
    }

    std::cout << "  [✓] Program loaded at 0x" << std::hex << System::ResetVector
              << std::dec << "\n";
    std::cout << "\n";
  } else {
    std::cout << "No program specified. Halting.\n";
    std::cout << "\nUsage: " << argv[0] << " <program.bin>\n";
    std::cout << "Example: " << argv[0] << " examples/test.bin\n";
    return 0;
  }

  /**
   * CPU RESET
   *
   * Initialize CPU state:
   * - PC = ResetVector (0x0)
   * - SP = InitialStackPointer (end of RAM)
   * - Clear all registers
   * - Flush pipeline
   */
  std::cout << "Resetting CPU...\n";
  cpu.Reset(System::ResetVector);
  std::cout << "  [✓] PC = 0x" << std::hex << cpu.GetPC() << std::dec << "\n";
  std::cout << "\n";

  /**
   * EXECUTION LOOP
   *
   * Run the CPU for a limited number of cycles or until HALT.
   * Each tick advances the CPU pipeline by one stage.
   *
   * NOTE (KleaSCM) In a real system this would run indefinitely
   * with interrupt handling. Here we limit to 10,000 cycles for safety.
   */
  std::cout << "Starting execution...\n";
  std::cout << "═══════════════════════════════════════════\n";

  const int MaxCycles = 10000;
  int cycles = 0;

  for (cycles = 0; cycles < MaxCycles; ++cycles) {
    // Tick CPU (advances pipeline)
    cpu.OnTick();

    // Tick Bus (processes pending transactions)
    bus.OnTick();

    // Check for halt condition
    // TODO: Add IsHalted() to CPU or check for specific PC values
    // For now, we run for a fixed number of cycles

    // Optional: Print PC every N cycles for debugging
    if (cycles % 100 == 0) {
      std::cout << "  Cycle " << cycles << " | PC = 0x" << std::hex
                << cpu.GetPC() << std::dec << "\n";
    }
  }

  std::cout << "═══════════════════════════════════════════\n";
  std::cout << "\n";

  /**
   * FINAL STATE REPORT
   *
   * Display CPU registers and statistics after execution.
   */
  std::cout << "Execution complete.\n";
  std::cout << "  Total Cycles: " << cycles << "\n";
  std::cout << "  Final PC: 0x" << std::hex << cpu.GetPC() << std::dec << "\n";
  std::cout << "\n";

  std::cout << "Register State:\n";
  for (int i = 0; i < 8; ++i) {
    std::cout << "  R" << i << " = 0x" << std::hex
              << cpu.GetRegister(static_cast<Cpu::Register>(i)) << std::dec
              << "\n";
  }
  std::cout << "\n";

  std::cout << "System halted gracefully. 💜\n";
  return 0;
}
