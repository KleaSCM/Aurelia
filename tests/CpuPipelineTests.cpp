/**
 * CPU Pipeline Integration Tests.
 *
 * Verifies the 5-stage pipeline FSM: Fetch -> Decode -> Execute -> Memory ->
 * WriteBack. Executes real machine code sequences.
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#include "Bus/Bus.hpp"
#include "Cpu/Cpu.hpp"
#include <catch2/catch_test_macros.hpp>
#include <map>

using namespace Aurelia::Cpu;
using namespace Aurelia::Bus;
using namespace Aurelia::Core;

// Simple RAM for storing instructions/data
class PipelineMemory : public IBusDevice {
public:
  std::map<Address, Data> Storage;
  Address Base = 0;
  Address Size = 0x10000;

  bool IsAddressInRange(Address addr) const override {
    return addr >= Base && addr < Base + Size;
  }

  bool OnRead(Address addr, Data &outData) override {
    if (Storage.contains(addr)) {
      outData = Storage[addr];
    } else {
      outData = 0; // NOP or Empty
    }
    // NOTE (KleaSCM) Zero-Wait state memory for pure pipeline logic
    // verification. The pipeline's handling of Wait states is verified in
    // separate integration tests.
    return true;
  }

  bool OnWrite(Address addr, Data inData) override {
    Storage[addr] = inData;
    return true;
  }

  void OnTick() override {}
};

class CpuPipelineTest {
protected:
  Aurelia::Bus::Bus bus;
  Cpu cpu;
  PipelineMemory mem;

public:
  CpuPipelineTest() {
    bus.ConnectDevice(&mem);
    cpu.ConnectBus(&bus);
    cpu.Reset(0);
  }
};

TEST_CASE_METHOD(CpuPipelineTest, "CpuPipeline - ExecuteADD") {
  // Program: ADD R1, R2, R3
  // R2 = 10, R3 = 20
  // Opcode ADD (0x01) | Rd(1) | Rn(2) | Rm(3) | 000
  // Raw: 0x01123000

  // Setup Registers
  cpu.SetRegister(Register::R2, 10);
  cpu.SetRegister(Register::R3, 20);

  // Load Instruction at 0
  mem.Storage[0] = 0x01123000;

  // Run Pipeline
  // Tick 1: Fetch (Req)
  cpu.OnTick();
  bus.OnTick();
  CHECK(cpu.GetState() == CpuState::Fetch);

  // Tick 2: Fetch (Data Ready) -> Decode
  cpu.OnTick();
  CHECK(cpu.GetState() == CpuState::Decode);

  // Tick 3: Decode -> Execute
  cpu.OnTick();
  CHECK(cpu.GetState() == CpuState::Execute);

  // Tick 4: Execute (ALU) -> WriteBack
  cpu.OnTick();
  CHECK(cpu.GetState() == CpuState::WriteBack);

  // Tick 5: WriteBack -> Fetch (Next PC=4)
  cpu.OnTick();
  CHECK(cpu.GetState() == CpuState::Fetch);
  CHECK(cpu.GetPC() == 4);

  // Verify Result
  CHECK(cpu.GetRegister(Register::R1) == 30);
}

TEST_CASE_METHOD(CpuPipelineTest, "CpuPipeline - ExecuteBranch") {
  // Program: B #8
  // Opcode B (0x30) | Imm=8
  // Raw: 0x30000008

  mem.Storage[0] = 0x30000008;

  // Fetch -> Decode -> Execute (Branch Taken) -> Fetch (Target)

  // 1. Fetch Req
  cpu.OnTick();
  bus.OnTick();
  // 2. Fetch Done -> Decode
  cpu.OnTick();
  // 3. Decode -> Execute
  cpu.OnTick();
  // 4. Execute (Branch) -> Fetch (New PC)
  cpu.OnTick();

  // PC should be 0 + 8 = 8
  CHECK(cpu.GetPC() == 8);
  CHECK(cpu.GetState() == CpuState::Fetch);
}

TEST_CASE_METHOD(CpuPipelineTest, "CpuPipeline - ExecuteLDR") {
  // Program: LDR R5, [R2, #0]
  // R2 = 0x100
  // Opcode LDR (0x10) | Rd(5) | Rn(2) | Imm(0)
  // Raw: 0x10520000

  // Setup Data
  cpu.SetRegister(Register::R2, 0x100);
  mem.Storage[0] = 0x10520000;     // Instruction at 0
  mem.Storage[0x100] = 0xDEADBEEF; // Data at 0x100

  // 1. Fetch Req
  cpu.OnTick();
  bus.OnTick();
  // 2. Fetch Done -> Decode
  cpu.OnTick();
  // 3. Decode -> Execute
  cpu.OnTick();
  // 4. Execute (Address Calc) -> Memory
  cpu.OnTick();
  CHECK(cpu.GetState() == CpuState::Memory);

  // 5. Memory Access Req
  cpu.OnTick();
  bus.OnTick();
  // 6. Memory Done -> WriteBack
  cpu.OnTick();
  CHECK(cpu.GetState() == CpuState::WriteBack);

  // 7. WriteBack -> Fetch
  cpu.OnTick();

  // Verify Load
  CHECK(cpu.GetRegister(Register::R5) == 0xDEADBEEF);
}
