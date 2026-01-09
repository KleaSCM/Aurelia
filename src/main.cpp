/**
 * Aurelia Virtual Machine Entry Point.
 *
 * Fully integrated System Emulator.
 * Supports running external binaries or a built-in Performance Benchmark Demo.
 *
 * Features:
 * - Full Hardware Emulation (CPU, RAM, UART, PIC, Timer, SSD).
 * - Real-time Performance Monitoring (MHz, Instructions, Time).
 * - Bus Traffic Analysis & Component Visualization.
 * - Integrated Assembler for on-the-fly demo generation.
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#include "Bus/Bus.hpp"
#include "Cpu/Cpu.hpp"
#include "Memory/RamDevice.hpp"
#include "Peripherals/PicDevice.hpp"
#include "Peripherals/TimerDevice.hpp"
#include "Peripherals/UartDevice.hpp"
#include "System/Loader.hpp"
#include "System/MemoryMap.hpp"
#include "Tools/Assembler/Encoder.hpp"
#include "Tools/Assembler/Lexer.hpp"
#include "Tools/Assembler/Parser.hpp"
#include "Tools/Assembler/Resolver.hpp"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <vector>

using namespace Aurelia;

// -----------------------------------------------------------------------------
// Assembler Helper
// -----------------------------------------------------------------------------
std::vector<std::uint8_t> Assemble(const std::string &Source) {
  using namespace Aurelia::Tools::Assembler;
  Lexer lexer(Source);
  auto tokens = lexer.Tokenize();
  if (tokens.empty())
    return {};

  Parser parser(tokens);
  if (!parser.Parse()) {
    std::cerr << "Parser Error: " << parser.GetErrorMessage() << "\n";
    return {};
  }

  auto instructions = parser.GetInstructions();
  auto labels = parser.GetLabels();
  Resolver resolver(instructions, labels);
  if (!resolver.Resolve()) {
    std::cerr << "Resolver Error: " << resolver.GetErrorMessage() << "\n";
    return {};
  }

  Encoder encoder(instructions);
  if (!encoder.Encode()) {
    std::cerr << "Encoder Error: " << encoder.GetErrorMessage() << "\n";
    return {};
  }
  return encoder.GetBinary();
}

// -----------------------------------------------------------------------------
// Built-in Demo Program (ASCII Pattern + SSD Writes)
// -----------------------------------------------------------------------------
std::vector<std::uint8_t> GenerateDemoProgram() {
  std::cout << "Generating Built-in Performance Benchmark (Mandelbrot-ish "
               "Pattern)...\n";
  // Logic: 20x60 loop generating ASCII art based on (X+Y)
  // PLUS: Verify SSD writing.
  std::string source = R"(
		; Setup UART Base Address (0xE0001000)
		MOV R1, #224
		MOV R2, #24
		LSL R1, R1, R2   ; R1 = 0xE0000000
		MOV R2, #16
		MOV R3, #8
		LSL R2, R2, R3   ; R2 = 0x1000
		ADD R1, R1, R2   ; R1 = 0xE0001000

		; Y Loop (20 lines)
		MOV R4, #20
	loop_y:
		; X Loop (60 chars)
		MOV R5, #60
		
	loop_x:
		; Calculate Char: (X + Y) & 63 + 33
		MOV R6, #0
		ADD R6, R4, R5
		MOV R7, #63
		AND R6, R6, R7
		MOV R7, #33
		ADD R6, R6, R7
		
		; Write Char to UART
		STR R6, [R1, #0]
		
		; Decrement X
		MOV R7, #1
		SUB R5, R5, R7
		MOV R6, #0
		CMP R5, R6
		BNE loop_x
		
		; Newline
		MOV R6, #10
		STR R6, [R1, #0]
		
		; Decrement Y
		MOV R7, #1
		SUB R4, R4, R7
		MOV R6, #0
		CMP R4, R6
		BNE loop_y
		
		; --------------------------
		; SSD Verification Step
		; --------------------------
		; Write 0xAA (170) to SSD Base (0xE0000000)
		MOV R8, #224
		MOV R9, #24
		LSL R8, R8, R9   ; R8 = 0xE0000000 (SSD Base)
		
		MOV R9, #170     ; Test Pattern
		STR R9, [R8, #0] ; Write to SSD Persistence
		
		HALT
	)";

  return Assemble(source);
}

// -----------------------------------------------------------------------------
// Main Application
// -----------------------------------------------------------------------------
void PrintBanner() {
  std::cout << "╔════════════════════════════════════════════╗\n"
            << "║     Aurelia Virtual System v0.1.0          ║\n"
            << "║     Modern CPU Emulator & Assembler        ║\n"
            << "╚════════════════════════════════════════════╝\n"
            << "\n";
}

int main(int argc, char *argv[]) {
  PrintBanner();

  // 1. Initialize Hardware
  std::cout << "Initializing Hardware...\n";
  Bus::Bus bus;
  Memory::RamDevice ram(System::RamSize, 0); // 256MB RAM
  Memory::RamDevice ssd(4096, 0);            // 4KB SSD Buffer
  ssd.SetBaseAddress(0xE0000000);            // SSD MMIO Base

  Cpu::Cpu cpu;
  Peripherals::UartDevice uart;   // 0xE0001000
  Peripherals::PicDevice pic;     // 0xE0002000
  Peripherals::TimerDevice timer; // 0xE0003000

  // 2. Wiring
  bus.ConnectDevice(&ram);
  bus.ConnectDevice(&ssd);
  bus.ConnectDevice(&uart);
  bus.ConnectDevice(&pic);
  bus.ConnectDevice(&timer);
  cpu.ConnectBus(&bus);

  std::cout << "  [✓] Bus Interconnect Active\n"
            << "  [✓] RAM: 256MB (Mapped @ 0x00000000)\n"
            << "  [✓] SSD: 4KB Buffer (Mapped @ 0xE0000000)\n"
            << "  [✓] CPU: Aurelia Core (Connected)\n"
            << "  [✓] Peripherals: UART, PIC, Timer\n"
            << "\n";

  // 3. Load Program
  std::vector<std::uint8_t> program;
  if (argc > 1) {
    if (std::string(argv[1]) == "--demo") {
      program = GenerateDemoProgram();
    } else {
      std::cout << "Loading binary: " << argv[1] << "...\n";
      System::Loader loader(bus);
      if (!loader.LoadBinary(argv[1], System::ResetVector)) {
        std::cerr << "Fatal: Failed to load binary.\n";
        return 1;
      }
    }
  } else {
    std::cout
        << "No input file provided. Defaulting to Internal Benchmark.\n\n";
    program = GenerateDemoProgram();
  }

  if (!program.empty()) {
    System::Loader loader(bus);
    if (!loader.LoadData(program, System::ResetVector)) {
      std::cerr << "Fatal: Failed to load program data.\n";
      return 1;
    }
  }

  // 4. Execution
  std::cout << "\nStarting Execution...\n";
  std::cout << "──────────────────────────────────────────────────\n";

  cpu.Reset(System::ResetVector);

  auto start = std::chrono::high_resolution_clock::now();
  std::uint64_t cycles = 0;
  const std::uint64_t MaxCycles = 5000000;

  while (!cpu.IsHalted() && cycles < MaxCycles) {
    cpu.OnTick();
    bus.OnTick();
    cycles++;
  }

  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsed = end - start;

  std::cout << "──────────────────────────────────────────────────\n";

  // 5. System Report
  double secs = elapsed.count();
  double mhz = (static_cast<double>(cycles) / secs) / 1000000.0;

  std::cout << "\nSYSTEM TELEMETRY REPORT:\n";
  std::cout << "  Performance:\n"
            << "    Clock Speed:     " << std::fixed << std::setprecision(2)
            << mhz << " MHz\n"
            << "    Exec Time:       " << std::fixed << std::setprecision(4)
            << secs << "s\n"
            << "    Total Cycles:    " << cycles << "\n";

  std::cout << "\n  Bus Traffic:\n"
            << "    Total Transfers: "
            << (bus.GetReadCount() + bus.GetWriteCount()) << "\n"
            << "    Memory Reads:    " << bus.GetReadCount() << "\n"
            << "    Memory Writes:   " << bus.GetWriteCount() << "\n";

  std::cout << "\n  Component Status:\n";

  // Check SSD
  Core::Data ssdVal = 0;
  // We use the raw RamDevice::OnRead to inspect (bypassing Bus to avoid
  // changing stats?) OnRead is const-safe usually, but here returns bool.
  // Actually, OnRead might have side effects? RamDevice::OnRead is pure read.
  if (ssd.OnRead(0xE0000000, ssdVal) && ssdVal == 170) {
    std::cout
        << "    SSD Persistence: [Verify OK] (Value 0xAA written to Disk)\n";
  } else {
    std::cout << "    SSD Persistence: [Idle] (No data detected)\n";
  }

  std::cout << "    CPU State:       "
            << (cpu.IsHalted() ? "HALTED" : "RUNNING") << "\n";
  std::cout << "    Final PC:        0x" << std::hex << cpu.GetPC() << std::dec
            << "\n";

  std::cout << "\nBye! 💜\n";

  return 0;
}
