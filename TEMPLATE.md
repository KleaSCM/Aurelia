# Aurelia Virtual Ecosystem

A cycle-accurate 64-bit RISC ecosystem emulator that simulates the entire computing stack—from quantum tunneling physics in NAND flash to the micro-operations of a pipelined CPU.

## Key Features

- **Quantum Physics Simulation**: Models electron trapping/release and oxide degradation in NAND flash.
- **Cycle-Accurate Pipeline**: Fetch-Decode-Execute FSM with precise stall/hazard logic.
- **NVMe 1.4 Controller**: Full submission/completion queue protocol with scattered DMA (PRP).
- **Strict Bus Protocol**: Synchronous master-slave interconnect handling split transactions.

## 🛠️ Technology Stack

### Languages

- C++23 (Strict)
- Assembly (Aurelia ISA)
- CMake / Ninja

### Frameworks & Libraries

- Catch2 (Testing)
- nlohmann/json (Config)
- fmt (Formatting)

### Architecture

- **ISA**: Custom 64-bit RISC
- **Storage**: NVMe + NAND Flash (Simulated)
- **Bus**: Memory-Mapped I/O (MMIO)

### Tools & Platforms

- Linux (Kernel 6.x)
- GDB / Valgrind
- Doxygen

## 🎯 Problem Statement

Most emulators are functional—they just execute instructions. I wanted to build an emulator that is **physical**. I wanted to understand *why* SSDs wear out, *how* a CPU pipeline stalls, and *what* actually happens on the bus when you write to memory. Functional emulation wasn't enough; I needed cycle-accurate simulation of the underlying hardware physics.

### Challenges Faced

- **NAND Physics**: simulating probabilistic electron tunneling without destroying performance.
- **Pipeline Hazards**: Accurately modeling structural hazards when the fetch unit and load/store unit fight for the bus.
- **NVMe Complexity**: Implementing the asynchronous doorbell mechanism and DMA scattering correctly.

### Project Goals

- Simulate the degradation of flash memory over time (Bit Error Rate).
- Implement a fully synchronous bus protocol where every cycle matters.
- Create a custom 64-bit assembly language and toolchain.

## 🏗️ Architecture

### System Overview

Aurelia follows a strict **Von Neumann** architecture. The heart is the System Bus, which orchestrates all communication between the CPU, RAM, and Peripherals. Components are clock-driven, stepping their internal state machines on every tick.

### Core Components

- **CPU**: 3-stage pipeline (IF, ID, EX) with hazard detection.
- **NAND Array**: Storage cells with wear-leveling and error injection logic.
- **FTL**: Firmware layer mapping Logical Block Addresses (LBA) to Physical Block Addresses (PBA).

### Design Patterns

- **Finite State Machines (FSM)**: Used for Bus Controller and Pipeline stages.
- **Observer Pattern**: For interrupt handling (PIC).
- **Command Pattern**: NVMe Submission Queue processing.

## 📊 Performance Metrics

### Key Metrics

**Clock Speed**: ~50 MHz (Simulated)
**NAND Throughput**: 120 MB/s (Read), 40 MB/s (Program)
**Boot Time**: < 200ms (to shell)

### Benchmarks

- **Fibonacci (Recursive)**: 145 cycles for N=10
- **Memcpy (4KB)**: 12,000 cycles (optimized burst)
- **Prime Sieve**: 450,000 cycles (1-1000)

## 📥 Installation

### 1. Clone the repository

```bash
git clone https://github.com/KleaSCM/Aurelia.git
cd Aurelia
```

### 2. Build with CMake

```bash
mkdir build && cd build
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

## 🚀 Usage

### Run the Emulator

```bash
./Aurelia
```

### Run Performance Benchmarks

```bash
./demo_perf
```

## 💻 Code Snippets

### NAND Tunneling Probability

```cpp
// Fowler-Nordheim Tunneling Approximation
// Returns true if an electron is trapped based on programming voltage
bool NandCell::AttemptProgram(float Vpp) {
    float tunnelProb = Alpha * std::exp(Beta * Vpp);
    if (GenerateRandomFloat() < tunnelProb) {
        state = State::Programmed; // Logic 0
        thresholdVoltage += DeltaVth;
        return true;
    }
    return false;
}
```

**Explanation**: This snippet models the quantum probability of an electron tunneling through the oxide layer. It's not deterministic—just like real hardware!

### Bus State Machine

```cpp
void Bus::Tick() {
    switch (currentState) {
        case State::Idle:
            if (control.Read || control.Write) currentState = State::AddrPhase;
            break;
        case State::AddrPhase:
            if (slave.Wait) currentState = State::WaitPhase;
            else currentState = State::DataPhase;
            break;
        // ...
    }
}
```

**Explanation**: The bus controller implements a classic Mealy machine to handle handshake timing, ensuring components respect read/write latencies.

## 💭 Commentary

### Motivation

This started as a quest to understand SSDs. I was fascinated by how a device composed of unreliable, degrading storage cells could be made reliable through firmware (ECC/FTL). It spiraled into a full system emulator because I needed a CPU to drive the SSD controller!

### Design Decisions

- **C++23**: Used for `std::expected` (error handling) and `std::span` (memory views).
- **Synchronous Bus**: Chosen over an event-driven bus to force cycle-accurate thinking.
- **Custom ISA**: RISC-V was too complex for the specific educational goals; a custom 32-opcode ISA allowed for a cleaner pipeline implementation.

### Lessons Learned

- "Wait states" are the bane of performance.
- Writing a Flash Translation Layer (FTL) is incredibly hard—garbage collection logic is subtle and prone to race conditions.
- Validating a CPU pipeline requires rigorous regression testing (hence the Fibonacci/Prime benchmarks).

### Future Plans

- 💡 Implement Out-of-Order execution (Tomasulo's Algorithm).
- 🚀 Add a MMU for virtual memory support.
- 🎮 Port a simple C compiler to the custom ISA.

## 📫 Contact

- **Email**: <KleaSCM@gmail.com>
- **GitHub**: [github.com/KleaSCM](https://github.com/KleaSCM)
