# Aurelia Virtual Ecosystem

<p align="center">
  <img src="https://img.shields.io/badge/C++-23-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white" />
  <img src="https://img.shields.io/badge/Status-Active-success?style=for-the-badge" />
  <img src="https://img.shields.io/badge/License-MIT-green?style=for-the-badge" />
</p>

<p align="center">
  <img src="https://img.shields.io/badge/CMake-064F8C?style=flat-square&logo=cmake&logoColor=white" />
  <img src="https://img.shields.io/badge/Ninja-333?style=flat-square" />
  <img src="https://img.shields.io/badge/NVMe-1.4-critical?style=flat-square" />
  <img src="https://img.shields.io/badge/Architecture-RISC-blueviolet?style=flat-square" />
</p>

**Aurelia** is a cycle-accurate ecosystem emulator modeling a custom 64-bit RISC architecture. It simulates the entire stack: from the quantum physics of NAND flash tunneling to the micro-operations of the CPU pipeline.

---

## Architecture

Aurelia models a classic **Von Neumann Architecture** with a strictly synchronous, bus-driven design.

```mermaid
graph TD
    CLK[System Clock] --> CPU
    CLK --> BUS
    CLK --> RAM
    
    subgraph "The Core"
        CPU[Aurelia CPU]
        REG[Register File (R0-R31)]
        ALU[ALU]
        CU[Control Unit]
        CPU <--> REG
        CPU <--> ALU
        CPU <--> CU
    end

    subgraph "The Nervous System"
        BUS((System Bus))
        CPU <==>|Address/Data| BUS
    end

    subgraph "Memory & Storage"
        RAM[DRAM Controller (256MB)]
        SSD[NVMe Storage Controller]
        NAND[(NAND Flash Array)]
        
        BUS <==> RAM
        BUS <==> SSD
        SSD <==> NAND
    end
    
    subgraph "Peripherals"
        UART[UART Console]
        PIC[Interrupt Controller]
        TIMER[System Timer]
        
        BUS <==> UART
        BUS <==> PIC
        BUS <==> TIMER
    end
```

---

## Key Features

### Core Architecture

- **Custom 64-bit RISC ISA**: Load/Store architecture with 32+ opcodes.
- **Pipeline Simulation**: Cycle-accurate 3-stage pipeline (Fetch-Decode-Execute) modeling structural and control hazards.
- **System Bus**: Synchronous master-slave interconnect handling 64-bit split-transaction transfers.

### Storage & Physics

- **NAND Flash Physics**: Simulates Fowler-Nordheim quantum tunneling for program/erase cycles and oxide degradation.
- **NVMe 1.4 Controller**: Implements submission/completion queue logic using Doorbell mechanisms.
- **Flash Translation Layer (FTL)**: Manages page-associative mapping, garbage collection, and wear leveling.

### Peripherals

- **Interrupt Controller (PIC)**: Programmable vector handling for asynchronous hardware events.
- **System Timer**: 64-bit reloadable countdown timer with interrupt generation.
- **UART Console**: Memory-mapped serial I/O for system communication.

## Documentation

The system is fully documented in the `docs/` directory:

| Document | Description |
|----------|-------------|
| [**Instructions (ISA)**](docs/specs/ISA_Reference.md) | Opcode encoding, ALU logic, and flag definitions. |
| [**Hardware Spec**](docs/specs/Hardware_Architecture.md) | Bus protocol timing, pipeline stages, and register maps. |
| [**NAND Physics**](docs/specs/NAND_Physics.md) | Tunneling equations, error correction (ECC), and FTL algorithms. |
| [**Assembly Guide**](docs/tutorials/Assembly_Guide.md) | EBNF grammar, directives, and manual ABI conventions. |

## Getting Started

### Prerequisites

- **Linux/Unix** Environment
- **C++23** Compiler (GCC 13+ / Clang 16+)
- **CMake** 3.25+
- **Ninja** Build System

### Build Instructions

```bash
# Clone the repository
git clone https://github.com/KleaSCM/Aurelia.git
cd Aurelia

# Configure and build
mkdir build && cd build
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

### Run the Emulator

```bash
# Run the standard emulator
./Aurelia

# Run the performance benchmark
./demo_perf
```

## Project Structure

```
Aurelia/
├── src/
│   ├── Core/           # Common types and utilities
│   ├── Cpu/            # Pipeline, ALU, Decoder, Register File
│   ├── Memory/         # DRAM Controller
│   ├── Storage/        # NVMe, NAND, FTL
│   ├── Bus/            # System Bus and Interconnects
│   ├── Peripherals/    # UART, PIC, Timer
│   ├── System/         # Board integration
│   └── Tools/          # Assembler and Disassembler
├── docs/               # Technical specifications
├── tests/              # Unit and integration tests
└── examples/           # Assembly programs and demos
```

## License

MIT License — see LICENSE file for details.

## Contact

For questions and support: <KleaSCM@gmail.com>
