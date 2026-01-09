# Aurelia Virtual Ecosystem 💜

> **Status**: v0.1.0 "The Awakening"
> **License**: MIT
> **Language**: C++23 (Strict)
> **Documentation**: [Full Library](docs/)

**Aurelia** is a cycle-accurate ecosystem emulator modeling a custom 64-bit RISC architecture. It simulates the entire stack: from the physics of NAND flash tunneling to the micro-operations of the CPU pipeline.

---

## Technical Reference Manual

We have compiled a massive, multi-volume specification for the system.

### [Volume 1: Instruction Set Architecture (ISA)](docs/specs/ISA_Reference.md)

* **32+ Opcodes** fully documented with bitwise encoding.
* **Mathematical Definitions** for every ALU operation ($R_d \leftarrow R_n + R_m$).
* **Flag Logic** (Z, N, C, V) detailed for each instruction.

### [Volume 2: Hardware Architecture](docs/specs/Hardware_Architecture.md)

* **Bus Protocol**: Timing diagrams for Read/Write transactions.
* **Pipeline Microarchitecture**: Fetch-Decode-Execute FSM logic.
* **Interrupt Controller**: Register maps and vector handling.

### [Volume 3: NAND Physics & Storage](docs/specs/NAND_Physics.md)

* **Quantum Tunneling Simulation**: Program/Erase cycle physics.
* **Wear Leveling Algorithms**: Mathematical models for FTL endurance.
* **NVMe Protocol**: Submission/Completion Queue logic.

### [Volume 4: Assembly Language Guide](docs/tutorials/Assembly_Guide.md)

* **EBNF Grammar**: Formal syntax definition.
* **Directives**: `.string`, `.byte` usage.
* **Examples**: Fibonacci, String manipulation kernels.

### [Aurelia Manual](Aurelia.md)

---

## System Architecture

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

## Getting Started

### Prerequisites

* **Linux/Unix** Environment
* **C++23** Compiler (GCC 13+ / Clang 16+)
* **CMake** 3.25+
* **Ninja**

### Build

```bash
mkdir build && cd build
cmake .. -G Ninja
cmake --build .
```

### Run the Demo

We have included a stunning **Performance Benchmark** demonstrating the CPU, Memory, and System Bus in real-time.

```bash
./Aurelia
```
