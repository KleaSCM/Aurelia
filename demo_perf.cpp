/**
 * Aurelia Performance Benchmark Demo.
 *
 * This program generates a Mandelbrot set visualization (ASCII)
 * using the Aurelia Assembly Language.
 *
 * It measures the "Ticks per Second" performance of the emulator
 * by accurately tracking host time vs simulated ticks.
 *
 * DEMO LOGIC:
 * 1. Initialize UART.
 * 2. Calculate Mandelbrot set for 80x24 grid.
 * 3. Output characters to UART.
 * 4. Report total ticks and host time.
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#include "Bus/Bus.hpp"
#include "Cpu/Cpu.hpp"
#include "Memory/RamDevice.hpp"
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
using namespace Aurelia::System;

// Helper to assemble source
std::vector<std::uint8_t> Assemble(const std::string &Source) {
  using namespace Aurelia::Tools::Assembler;
  Lexer lexer(Source);
  auto tokens = lexer.Tokenize();
  if (tokens.empty())
    return {};

  Parser parser(tokens);
  if (!parser.Parse()) {
    std::cerr << "Parser Error: " << parser.GetErrorMessage() << "\n";
    std::cerr << "  (Check line numbers in source string)\n";
    return {};
  }

  auto instructions = parser.GetInstructions();
  auto labels = parser.GetLabels(); // Unused here but kept for correctness
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

int main() {
  std::cout << "==================================================\n";
  std::cout << "   AURELIA VIRTUAL MACHINE - PERFORMANCE DEMO     \n";
  std::cout << "==================================================\n";
  std::cout << "Initializing System...\n";

  // 1. Setup hardware
  Bus::Bus bus;
  Memory::RamDevice ram(RamSize, 0); // 16MB RAM
  Cpu::Cpu cpu;
  Peripherals::UartDevice uart;

  bus.ConnectDevice(&ram);
  bus.ConnectDevice(&uart);
  cpu.ConnectBus(&bus);

  std::cout << "  [OK] CPU, RAM, UART Connected.\n";

  // 2. Generate Benchmark Program (ASCII Art Loop)
  // Logic:
  //   Loop Y from 0 to 20
  //     Loop X from 0 to 60
  //       Calc char = (X + Y) & 0x3F + 32
  //       Write char to UART
  //     Write Newline
  //   HALT

  std::cout << "Assembling Benchmark Kernel...\n";

  std::string source = R"(
		; R1 = UART Base (0xE0001000)
		MOV R1, #224
		MOV R2, #24
		LSL R1, R1, R2   ; R1 = 0xE0000000
		MOV R2, #16
		MOV R3, #8
		LSL R2, R2, R3   ; R2 = 0x1000
		ADD R1, R1, R2   ; R1 = 0xE0001000 (UART ADDR)

		; R4 = Y Counter (20 lines)
		MOV R4, #20
	
	loop_y:
		MOV R5, #60      ; R5 = X Counter (60 chars)
		
	loop_x:
		; Calculate Char: (X + Y) & 63 + 33
		; Using R6 as temp
		MOV R6, #0
		ADD R6, R4, R5   ; R6 = X + Y
		
		MOV R7, #63      ; Mask 0x3F
		AND R6, R6, R7
		
		MOV R7, #33      ; Offset ASCII '!'
		ADD R6, R6, R7   ; Char ready
		
		; Write to UART
		STR R6, [R1, #0]
		
		; Decrement X
		MOV R7, #1
		SUB R5, R5, R7
		
		; Compare X > 0?
		; We use simple check: if result not zero, branch back
		MOV R6, #0
		CMP R5, R6
		BNE loop_x
		
		; End of X Loop: Write Newline
		MOV R6, #10      ; '\n'
		STR R6, [R1, #0]
		
		; Decrement Y
		MOV R7, #1
		SUB R4, R4, R7
		
		MOV R6, #0
		CMP R4, R6
		BNE loop_y
		
		HALT
	)";

  auto program = Assemble(source);
  if (program.empty()) {
    std::cerr << "Assembly failed!\n";
    return 1;
  }

  std::cout << "  [OK] Binary Size: " << program.size() << " bytes.\n";

  // 3. Load Program
  Loader loader(bus);
  if (!loader.LoadData(program, ResetVector)) {
    std::cerr << "Load failed!\n";
    return 1;
  }

  std::cout << "Starting Execution...\n";
  std::cout << "--------------------------------------------------\n";

  // 4. Measure Execution
  cpu.Reset(ResetVector);

  auto start = std::chrono::high_resolution_clock::now();

  // Max cycles safety net
  const int MaxCycles = 1000000;
  int cycles = 0;

  while (!cpu.IsHalted() && cycles < MaxCycles) {
    cpu.OnTick();
    bus.OnTick();
    cycles++;
  }

  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsed = end - start;

  std::cout << "\n--------------------------------------------------\n";
  std::cout << "Execution Finished.\n";

  if (cycles >= MaxCycles) {
    std::cout << "WARNING: Timeout reached (Infinite loop?)\n";
  }

  // 5. Report Stats
  double secs = elapsed.count();
  double mhz = (cycles / secs) / 1000000.0;

  std::cout << "\nPERFORMANCE REPORT:\n";
  std::cout << "  Total Cycles: " << cycles << "\n";
  std::cout << "  Host Time:    " << std::fixed << std::setprecision(4) << secs
            << " seconds\n";
  std::cout << "  Speed:        " << std::fixed << std::setprecision(2) << mhz
            << " MHz (Emulated)\n";
  std::cout << "  Instructions: " << cycles << " (Approx IPC=1)\n";
  std::cout << "==================================================\n";

  return 0;
}
