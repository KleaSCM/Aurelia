/**
 * Aurelia Virtual Machine Entry Point.
 *
 * Fully integrated System Emulator & Performance Harness.
 *
 * This file serves as the orchestration layer for the Aurelia Virtual SoC.
 * It instantiates the critical hardware components (CPU, Bus, RAM,
 * Peripherals), establishes the interconnect wiring, and drives the main
 * execution clock loop.
 *
 * DESIGN PHILOSOPHY:
 * Unlike event-driven simulators (e.g., QEMU), Aurelia uses a cycle-accurate
 * "Tick" loop. This simplifies the architectural model and ensures
 * deterministic behavior for debugging pipeline hazards and bus contention, at
 * the cost of raw throughput.
 *
 * SYSTEM ARCHITECTURE:
 * ┌───────────────┐      ┌───────────────┐      ┌───────────────┐
 * │  Aurelia CPU  │◄────►│  System Bus   │◄────►│  RAM (256MB)  │
 * └───────┬───────┘      └───────┬───────┘      └───────┬───────┘
 *         │                      │                      │
 *         ▼                      ▼                      ▼
 * ┌───────────────┐      ┌───────────────┐      ┌───────────────┐
 * │  UART (TTY)   │      │      PIC      │      │     Timer     │
 * └───────────────┘      └───────────────┘      └───────────────┘
 *
 * PERFORMANCE METRICS:
 * The harness captures real-time telemetry including:
 * - Effective Clock Rate (MHz)
 * - IPC (Instructions Per Cycle) via Bus Utilization
 * - Total Execution Time
 *
 * USAGE:
 * $ ./aurelia_vm [binary_path]
 * $ ./aurelia_vm --demo  (Runs internal micro-benchmark)
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

/**
 * @brief Just-In-Time Assembler Utility.
 *
 * Converts raw assembly source strings into executable machine code binaries
 * used for internal benchmarks and unit tests.
 *
 * PIPELINE:
 * [Source] -> Lexer -> [Tokens] -> Parser -> [AST] -> Resolver -> [Final AST]
 * -> Encoder -> [Binary]
 *
 * @param Source The assembly source code string (NASM/GAS syntax derivative).
 * @return std::vector<uint8_t> The compiled binary blob (Machine Code).
 */
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

/**
 * @brief Generates the 'Mandelbrot-ish' Performance Benchmark.
 *
 * Creates a synthetic workload designed to stress the CPU Pipeline and
 * Bus Interconnect.
 *
 * WORKLOAD CHARACTERISTICS:
 * - 20x60 Nested Loop (O(N^2) complexity).
 * - Heavy Arithmetic usage (ADD, SUB, MUL).
 * - High density of Branch instructions (Pipeline Flush stress test).
 * - MMIO Store operations to UART (Bus Write serialization).
 *
 * @return std::vector<uint8_t> The benchmark binary.
 */
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

/**
 * @brief Prints the startup banner to stdout.
 *
 * Visual confirmation of system initialization.
 */
void PrintBanner() {
  std::cout << "╔════════════════════════════════════════════╗\n"
            << "║     Aurelia Virtual System v0.1.0          ║\n"
            << "║     Modern CPU Emulator & Assembler        ║\n"
            << "╚════════════════════════════════════════════╝\n"
            << "\n";
}

/**
 * @brief Application Entry Point.
 *
 * Sequences the emulator lifecycle:
 * 1. Hardware Initialization (Allocation of Device Objects).
 * 2. Wiring (Interconnect topology definition).
 * 3. Program Loading (External Binary vs Internal Demo).
 * 4. Execution Loop (Fetch-Decode-Execute Cycle).
 * 5. Telemetry Reporting (Performance Stats).
 *
 * @param argc Argument count.
 * @param argv Argument vector (argv[1] = optional binary path).
 * @return int 0 on success, 1 on load failure.
 */
int main(int argc, char *argv[]) {
  PrintBanner();

  // -------------------------------------------------------------------------
  // 1. Hardware Initialization
  // -------------------------------------------------------------------------
  std::cout << "Initializing Hardware...\n";
  Bus::Bus bus;
  Memory::RamDevice ram(System::RamSize, 0); // 256MB RAM
  Memory::RamDevice ssd(4096, 0);            // 4KB SSD Buffer
  ssd.SetBaseAddress(0xE0000000);            // SSD MMIO Base

  Cpu::Cpu cpu;
  Peripherals::UartDevice uart;   // 0xE0001000
  Peripherals::PicDevice pic;     // 0xE0002000
  Peripherals::TimerDevice timer; // 0xE0003000

  // -------------------------------------------------------------------------
  // 2. Component Wiring (Bus Topology)
  // -------------------------------------------------------------------------
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

  // -------------------------------------------------------------------------
  // 3. Program Loader
  // -------------------------------------------------------------------------
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

  // -------------------------------------------------------------------------
  // 4. Main Execution Loop
  // -------------------------------------------------------------------------
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

  // -------------------------------------------------------------------------
  // 5. System Telemetry Report
  // -------------------------------------------------------------------------
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

  // NOTE (KleaSCM): Direct memory access via Device::OnRead() to bypass
  // Bus tracking. This prevents observation from altering performance stats.
  Core::Data ssdVal = 0;
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
