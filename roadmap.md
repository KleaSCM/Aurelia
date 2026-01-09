# Aurelia - Comprehensive Technical Roadmap

> [!IMPORTANT]
> **Status**: PLANNING
> **Scope**: Full Ecosystem Emulation (CPU, Bus, RAM, SSD/FTL, IO)
> **Stack**: C++23, CMake, Ninja

## 1. Global Infrastructure & Tooling

- [ ] **1.1 Build System Setup**
  - [ ] Create `CMakeLists.txt` project root.
  - [ ] Add `set(CMAKE_CXX_STANDARD 23)` and `set(CMAKE_CXX_STANDARD_REQUIRED ON)`.
  - [ ] Configure Strict Flags: `-Wall -Wextra -Werror -pedantic -Wconversion`.
  - [ ] Setup `src/`, `tests/`, `external/` directory structure.
  - [ ] Integrate **Catch2** or **GTest** in `tests/CMakeLists.txt`.
  - [ ] Create `run_tests` target.
- [ ] **1.2 Common Types & Primitives (`Core/`)**
  - [ ] Create `src/Core/Types.hpp`.
  - [ ] Define `using Address = uint64_t;`.
  - [ ] Define `using Data = uint64_t;`.
  - [ ] Define `using Byte = uint8_t;`.
  - [ ] Define `using Word = uint64_t;`.
  - [ ] Create `src/Core/BitManip.hpp` (Templates for bit masking/shifting).
  - [ ] Implement `SetBit`, `ClearBit`, `CheckBit`, `ExtractBits` templates.
- [ ] **1.3 System Clock**
  - [ ] Create `class Clock`.
  - [ ] Implement `Clock::Tick()` (increments internal `uint64_t counter`).
  - [ ] Implement `Clock::GetTotalTicks()`.
  - [ ] Define `interface ITickable`.
  - [ ] Define pure virtual `void OnTick()` in `ITickable`.
  - [ ] Implement `System` class to hold `std::vector<ITickable*>`.
  - [ ] Implement `System::Run(cycles)` loop.

## 2. Bus System (The Nervous System)

- [ ] **2.1 Bus Definitions (`Bus/`)**
  - [ ] Create `src/Bus/BusDefs.hpp`.
  - [ ] Define `enum class ControlSignal { READ, WRITE, READY, WAIT, IRQ }`.
  - [ ] Define `struct BusState` (Address, Data, Control Lines).
- [ ] **2.2 Device Interface**
  - [ ] Create `class IBusDevice` inheriting `ITickable`.
  - [ ] Add pure virtual `bool IsAddressInRange(Address addr)`.
  - [ ] Add pure virtual `void OnRead(Address addr, Data& outData)`.
  - [ ] Add pure virtual `void OnWrite(Address addr, Data inData)`.
- [ ] **2.3 Bus Implementation**
  - [ ] Create `class Bus`.
  - [ ] Implement `Bus::ConnectDevice(IBusDevice* device)`.
  - [ ] Implement `Bus::SetAddress(Address addr)`.
  - [ ] Implement `Bus::SetData(Data data)`.
  - [ ] Implement `Bus::SetControl(ControlSignal signal, bool state)`.
  - [ ] Implement `Bus::Tick()` state machine:
    - [ ] Check Master request (Read/Write).
    - [ ] Broadcast Address to all devices.
    - [ ] Select Device (based on `IsAddressInRange`).
    - [ ] Handle `WAIT` signal from selected Device.
    - [ ] Commit Data transfer on `!WAIT` (Ready).

## 3. The Memory Subsystem (RAM)

- [ ] **3.1 RAM Controller (`Memory/`)**
  - [ ] Create `class RamDevice` inheriting `IBusDevice`.
  - [ ] Define `std::unique_ptr<Byte[]> memory_array`.
  - [ ] Implement `RamDevice::Initialize(size_t capacity)`.
  - [ ] Implement bounds checking logic.
- [ ] **3.2 Latency Simulation**
  - [ ] Add `latency_counter` to `RamDevice`.
  - [ ] Implement `OnTick` to decrement `latency_counter` when active.
  - [ ] Assert `WAIT` signal on Bus while `latency_counter > 0`.
  - [ ] Release `WAIT` when counter hits 0.

## 4. The Storage Subsystem (SSD & NAND)

- [ ] **4.1 NAND Physics Layer (`Storage/Nand/`)**
  - [ ] Define `struct Page` (4KB Data + 64B OOB).
  - [ ] Define `class Block` containing `std::array<Page, 64>`.
  - [ ] Define `class Plane` containing `std::vector<Block>`.
  - [ ] Implement `NandCell::Program(byte)` logic (AND operation: 1->0 allowed, 0->1 forbidden).
  - [ ] Implement `Block::Erase()` logic (resets all bits to 1).
- [ ] **4.2 Flash Translation Layer (FTL) (`Storage/FTL/`)**
  - [ ] Create `class LogicalToPhysicalMap`.
  - [ ] Implement `Map::Get(LBA)` -> `PBA`.
  - [ ] Implement `Map::Set(LBA, PBA)`.
  - [ ] Implement `FreeBlockList` manager.
  - [ ] Implement `GarbageCollector::FindVictimBlock()`.
  - [ ] Implement `GarbageCollector::RelocateValidPages()`.
- [ ] **4.3 Storage Controller (`Storage/Controller/`)**
  - [ ] Create `class SsdController` inheriting `IBusDevice`.
  - [ ] Implement NVMe-style Command Registers (Submission/Completion Pointers).
  - [ ] Implement `ProcessCommand(Read)`: LBA -> FTL -> NAND Read -> Bus DMA.
  - [ ] Implement `ProcessCommand(Write)`: Bus DMA -> Buffer -> FTL -> NAND Program.

## 5. The Central Processing Unit (CPU)

- [ ] **5.1 Register File (`Cpu/`)**
  - [ ] Create `class RegisterFile`.
  - [ ] Define 16 `uint64_t` registers.
  - [ ] Define `ProgramCounter` and `StackPointer`.
  - [ ] Define `StatusRegister` with Flag bitfields.
- [ ] **5.2 Arithmetic Logic Unit (ALU)**
  - [ ] Create `class Alu`.
  - [ ] Implement `Add(a, b)` with Carry/Overflow detection.
  - [ ] Implement `Sub(a, b)` with Borrow detection.
  - [ ] Implement `BitwiseAnd`, `BitwiseOr`, `BitwiseXor`.
  - [ ] Implement `ShiftLeft`, `ShiftRight` (Logical & Arithmetic).
- [ ] **5.3 Instruction Set Decoder**
  - [ ] Define `enum class Opcode`.
  - [ ] Create `struct Instruction` (Opcode, Registers, Immediate).
  - [ ] Implement `Decoder::Decode(uint32_t raw)` -> `Instruction`.
- [ ] **5.4 Control Unit Pipeline**
  - [ ] Create `class ControlUnit`.
  - [ ] Implement `Fetch()`: Send PC to Bus, wait for Data.
  - [ ] Implement `Decode()`: Call Decoder.
  - [ ] Implement `Execute()`: Route data to ALU or load/store unit.
  - [ ] Implement `MemoryAccess()`: Interact with Bus for Load/Store ops.
  - [ ] Implement `WriteBack()`: Update Register File.

## 6. Verification & System Integration

- [ ] **6.1 Assembler Tool**
  - [ ] Parse text files (e.g., `ADD R1, R2, R3`).
  - [ ] Generate binary machine code.
- [ ] **6.2 System Tests**
  - [ ] Write `IntegrationTest_BootProcess`.
  - [ ] Write `IntegrationTest_RamReadWrite`.
  - [ ] Write `IntegrationTest_SsdPersistence` (Write data, restart, read back).
