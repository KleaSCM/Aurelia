# Aurelia - Comprehensive Technical Roadmap

> [!IMPORTANT]
> **Status**: EXECUTION
> **Scope**: Full Ecosystem Emulation (CPU, Bus, RAM, SSD/FTL, IO)
> **Stack**: C++23, CMake, Ninja

## 1. Global Infrastructure & Tooling

- [x] **1.1 Build System Setup**
  - [x] Create `CMakeLists.txt` project root.
  - [x] Add `set(CMAKE_CXX_STANDARD 23)` and `set(CMAKE_CXX_STANDARD_REQUIRED ON)`.
  - [x] Configure Strict Flags: `-Wall -Wextra -Werror -pedantic -Wconversion`.
  - [x] Setup `src/`, `tests/`, `external/` directory structure.
  - [x] Integrate **Catch2** or **GTest** in `tests/CMakeLists.txt`.
  - [x] Create `run_tests` target.
- [x] **1.2 Common Types & Primitives (`Core/`)**
  - [x] Create `src/Core/Types.hpp`.
  - [x] Define `using Address = uint64_t;`.
  - [x] Define `using Data = uint64_t;`.
  - [x] Define `using Byte = uint8_t;`.
  - [x] Define `using Word = uint64_t;`.
  - [x] Create `src/Core/BitManip.hpp` (Templates for bit masking/shifting).
  - [x] Implement `SetBit`, `ClearBit`, `CheckBit`, `ExtractBits` templates.
- [x] **1.3 System Clock**
  - [x] Create `class Clock`.
  - [x] Implement `Clock::Tick()` (increments internal `uint64_t counter`).
  - [x] Implement `Clock::GetTotalTicks()`.
  - [x] Define `interface ITickable`.
  - [x] Define pure virtual `void OnTick()` in `ITickable`.
  - [x] Implement `System` class to hold `std::vector<ITickable*>`.
  - [x] Implement `System::Run(cycles)` loop.

## 2. Bus System (The Nervous System)

- [x] **2.1 Bus Definitions (`Bus/`)**
  - [x] Create `src/Bus/BusDefs.hpp`.
  - [x] Define `enum class ControlSignal { READ, WRITE, READY, WAIT, IRQ }`.
  - [x] Define `struct BusState` (Address, Data, Control Lines).
- [x] **2.2 Device Interface**
  - [x] Create `class IBusDevice` inheriting `ITickable`.
  - [x] Add pure virtual `bool IsAddressInRange(Address addr)`.
  - [x] Add pure virtual `void OnRead(Address addr, Data& outData)`.
  - [x] Add pure virtual `void OnWrite(Address addr, Data inData)`.
- [x] **2.3 Bus Implementation**
  - [x] Create `class Bus`.
  - [x] Implement `Bus::ConnectDevice(IBusDevice* device)`.
  - [x] Implement `Bus::SetAddress(Address addr)`.
  - [x] Implement `Bus::SetData(Data data)`.
  - [x] Implement `Bus::SetControl(ControlSignal signal, bool state)`.
  - [x] Implement `Bus::Tick()` state machine:
    - [x] Check Master request (Read/Write).
    - [x] Broadcast Address to all devices.
    - [x] Select Device (based on `IsAddressInRange`).
    - [x] Handle `WAIT` signal from selected Device.
    - [x] Commit Data transfer on `!WAIT` (Ready).

## 3. The Memory Subsystem (RAM)

- [x] **3.1 RAM Controller (`Memory/`)**
  - [x] Create `class RamDevice` inheriting `IBusDevice`.
  - [x] Define `std::unique_ptr<Byte[]> memory_array`.
  - [x] Implement `RamDevice::Initialize(size_t capacity)`.
  - [x] Implement bounds checking logic.
- [x] **3.2 Latency Simulation**
  - [x] Add `latency_counter` to `RamDevice`.
  - [x] Implement `OnTick` to decrement `latency_counter` when active.
  - [x] Assert `WAIT` signal on Bus while `latency_counter > 0`.
  - [x] Release `WAIT` when counter hits 0.

## 4. The Storage Subsystem (SSD & NAND)

- [x] **4.1 NAND Physics Layer (`Storage/Nand/`)**
  - [x] Define `struct Page` (4KB Data + 64B OOB).
  - [x] Define `class Block` containing `std::array<Page, 64>`.
  - [x] Define `class Plane` containing `std::vector<Block>`.
  - [x] Implement `NandCell::Program(byte)` logic (AND operation: 1->0 allowed, 0->1 forbidden).
  - [x] Implement `Block::Erase()` logic (resets all bits to 1).
- [x] **4.2 Flash Translation Layer (FTL) (`Storage/FTL/`)**
  - [x] Create `class LogicalToPhysicalMap`.
  - [x] Implement `Map::Get(LBA)` -> `PBA`.
  - [x] Implement `Map::Set(LBA, PBA)`.
  - [x] Implement `FreeBlockList` manager.
  - [x] Implement `GarbageCollector::FindVictimBlock()`.
  - [x] Implement `GarbageCollector::RelocateValidPages()`.
- [x] **4.3 Storage Controller (`Storage/Controller/`)**
  - [x] Create `class StorageController` inheriting `IBusDevice`.
  - [x] Implement NVMe Register Map (CAP, CC, CSTS, ASQ, ACQ, Doorbells).
  - [x] Implement `FetchCommand`: DMA Read SQE from Host RAM.
  - [x] Implement `ExecuteCommand`: DMA Data Transfer (Host <-> Device) + FTL.
  - [x] Implement `PostCompletion`: DMA Write CQE to Host RAM.

## 5. The Central Processing Unit (CPU)

- [x] **5.1 Register File (`Cpu/`)**
  - [x] Create `class RegisterFile`.
  - [x] Define 16 `uint64_t` registers.
  - [x] Define `ProgramCounter` and `StackPointer`.
  - [x] Define `StatusRegister` with Flag bitfields.
- [x] **5.2 Arithmetic Logic Unit (ALU)**
  - [x] Create `class Alu`.
  - [x] Implement `Add(a, b)` with Carry/Overflow detection.
  - [x] Implement `Sub(a, b)` with Borrow detection.
  - [x] Implement `BitwiseAnd`, `BitwiseOr`, `BitwiseXor`.
  - [x] Implement `ShiftLeft`, `ShiftRight` (Logical & Arithmetic).
- [x] **5.3 Instruction Set Decoder**
  - [x] Define `enum class Opcode`.
  - [x] Create `struct Instruction` (Opcode, Registers, Immediate).
  - [x] Implement `Decoder::Decode(uint32_t raw)` -> `Instruction`.
- [x] **5.4 Control Unit Pipeline**
  - [x] Create `class Cpu` (Control Unit).
  - [x] Implement `Fetch`: Request PC from Bus, wait for Wait=0.
  - [x] Implement `Decode`: Parse Opcode/Operands.
  - [x] Implement `Execute`: Dispatch to ALU or Branch logic.
  - [x] Implement `Memory`: Load/Store Data Phase.
  - [x] Implement `WriteBack`: Commit to Register File.

## 6. Verification & System Integration

- [ ] **6.1 Assembler Tool**
  - [ ] Parse text files (e.g., `ADD R1, R2, R3`).
  - [ ] Generate binary machine code.
- [ ] **6.2 System Tests**
  - [x] `NvmeControllerTest`: Full Host-Controller-NAND Loop.
  - [x] `CpuPipelineTest`: Pipeline FSM Execution.
  - [x] `FtlPersistenceTest`: Power-loss recovery verification.
