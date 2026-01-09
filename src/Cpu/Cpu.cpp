/**
 * CPU Implementation (Pipeline FSM).
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#include <cstring>

#include "Core/BitManip.hpp"
#include "Cpu/Alu.hpp"
#include "Cpu/Cpu.hpp"
#include "Cpu/Decoder.hpp"

namespace Aurelia::Cpu {

Cpu::Cpu() { GPR.fill(0); }

void Cpu::ConnectBus(Bus::Bus *Bus) { SystemBus = Bus; }

void Cpu::Reset(Core::Address StartAddress) {
  PC = StartAddress;
  State = CpuState::Fetch;
  CurrentFlags = {};
  GPR.fill(0);
  MicroOp = 0;
  Halted = false;
}

Core::Word Cpu::GetRegister(Register Reg) const {
  return GPR[static_cast<std::size_t>(Reg)];
}

void Cpu::SetRegister(Register Reg, Core::Word Value) {
  GPR[static_cast<std::size_t>(Reg)] = Value;
}

Core::Address Cpu::GetPC() const { return PC; }

void Cpu::SetPC(Core::Address Value) { PC = Value; }

const Flags &Cpu::GetFlags() const { return CurrentFlags; }

void Cpu::OnTick() {
  if (!SystemBus || Halted) {
    return;
  }

  switch (State) {
  case CpuState::Fetch: {
    if (MicroOp == 0) {
      /**
       * FETCH PHASE 1: REQUEST
       * Initiate Read transaction on the Bus at current PC address.
       */
      SystemBus->SetAddress(PC);
      SystemBus->SetControl(Bus::ControlSignal::Read, true);
      SystemBus->SetControl(Bus::ControlSignal::Write, false);
      MicroOp = 1;
    } else {
      /**
       * FETCH PHASE 2: WAIT
       * Wait for Bus to de-assert WAIT signal, indicating Valid Data.
       * Latch data into instruction buffer and move to Decode.
       */
      auto busState = SystemBus->GetState();
      if (!Core::CheckBit(busState.Control, 3)) { // Bit 3 = WAIT
        // Memory Ready. Latches Instruction
        std::uint32_t rawInstr = static_cast<std::uint32_t>(busState.DataBus);
        CurrentInstr = Decoder::Decode(rawInstr);

        // Clear Bus Request
        SystemBus->SetControl(Bus::ControlSignal::Read, false);

        State = CpuState::Decode;
        MicroOp = 0;
      }
    }
    break;
  }

  case CpuState::Decode: {
    /**
     * DECODE STAGE
     *
     * Prepares operands (OpA, OpB) for the ALU or Address Generation.
     * Reads from Register File based on Instruction Type.
     */
    if (CurrentInstr.Type == InstrType::Register) {
      OpA = GetRegister(CurrentInstr.Rn);
      OpB = GetRegister(CurrentInstr.Rm);
    } else if (CurrentInstr.Type == InstrType::Immediate) {
      OpA =
          GetRegister(CurrentInstr.Rn); // Base for LDR/STR, or ignored for MOV
      OpB = CurrentInstr.Immediate;
    } else if (CurrentInstr.Type == InstrType::Branch) {
      // Branch Offset
      OpB = CurrentInstr.Immediate;
      // Sign Extend 11-bit Immediate
      if (OpB & 0x400) {
        OpB |= 0xFFFFFFFFFFFFF800;
      }
    }

    State = CpuState::Execute;
    break;
  }

  case CpuState::Execute: {
    /**
     * EXECUTE STAGE
     *
     * Performs ALU operations, evaluates Branch conditions, and calculates
     * Addresses.
     */
    AluOp operation = AluOp::ADD; // Default

    switch (CurrentInstr.Op) {
    case Opcode::ADD:
      operation = AluOp::ADD;
      break;
    case Opcode::SUB:
      operation = AluOp::SUB;
      break;
    case Opcode::AND:
      operation = AluOp::AND;
      break;
    case Opcode::OR:
      operation = AluOp::OR;
      break;
    case Opcode::XOR:
      operation = AluOp::XOR;
      break;
    case Opcode::LSL:
      operation = AluOp::LSL;
      break;
    case Opcode::LSR:
      operation = AluOp::LSR;
      break;
    case Opcode::MOV:
      // MOV is effectively OPA=0 + OPB
      OpA = 0;
      operation = AluOp::ADD;
      break;
    case Opcode::CMP:
      operation = AluOp::SUB;
      break;
    case Opcode::Halt:
      // HALT instruction - stop execution
      Halted = true;
      return;
    default:
      break;
    }

    if (CurrentInstr.Type == InstrType::Branch) {
      // Handle Branch Logic
      bool takeBranch = false;
      if (CurrentInstr.Op == Opcode::B) {
        takeBranch = true;
      } else if (CurrentInstr.Op == Opcode::BEQ && CurrentFlags.Z) {
        takeBranch = true;
      } else if (CurrentInstr.Op == Opcode::BNE && !CurrentFlags.Z) {
        takeBranch = true;
      }

      if (takeBranch) {
        PC += OpB;               // Relative Branch
        State = CpuState::Fetch; // Flush pipeline (simplification: just go to
                                 // fetch)
        MicroOp = 0;
        return;
      }
    } else if (CurrentInstr.Op == Opcode::LDR ||
               CurrentInstr.Op == Opcode::STR) {
      // Calculate Address: Rn + Imm
      AluResult = OpA + OpB; // Simple address calc
      State = CpuState::Memory;
      MicroOp = 0;
      return;
    } else {
      // ALU Operation
      auto res = Alu::Execute(operation, OpA, OpB, CurrentFlags);
      AluResult = res.Result;
      CurrentFlags = res.NewFlags;
    }

    State = CpuState::WriteBack;
    break;
  }

  case CpuState::Memory: {
    /**
     * MEMORY STAGE
     *
     * Handles Data Load/Store transactions using the calculated effective
     * address. Multi-cycle operation utilizing the Bus Wait signal.
     */
    if (MicroOp == 0) {
      if (CurrentInstr.Op == Opcode::LDR) {
        SystemBus->SetAddress(AluResult);
        SystemBus->SetControl(Bus::ControlSignal::Read, true);
        SystemBus->SetControl(Bus::ControlSignal::Write, false);
      } else if (CurrentInstr.Op == Opcode::STR) {
        SystemBus->SetAddress(AluResult);
        SystemBus->SetData(GetRegister(CurrentInstr.Rd)); // Store Value
        SystemBus->SetControl(Bus::ControlSignal::Write, true);
        SystemBus->SetControl(Bus::ControlSignal::Read, false);
      }
      MicroOp = 1;
    } else {
      auto busState = SystemBus->GetState();
      if (!Core::CheckBit(busState.Control, 3)) { // Wait cleared
        if (CurrentInstr.Op == Opcode::LDR) {
          MemData = busState.DataBus;
          SystemBus->SetControl(Bus::ControlSignal::Read, false);
        } else {
          SystemBus->SetControl(Bus::ControlSignal::Write, false);
        }
        State = CpuState::WriteBack;
        MicroOp = 0;
      }
    }
    break;
  }

  case CpuState::WriteBack: {
    /**
     * WRITEBACK STAGE
     *
     * Commits results to the Register File.
     * Updates PC to next instruction.
     */
    if (CurrentInstr.Op == Opcode::LDR) {
      SetRegister(CurrentInstr.Rd, MemData);
    } else if (CurrentInstr.Op != Opcode::STR &&
               CurrentInstr.Op != Opcode::CMP &&
               CurrentInstr.Type != InstrType::Branch) {
      // ALU Result Writeback
      SetRegister(CurrentInstr.Rd, AluResult);
    }

    // Increment PC (if not branched)
    PC += 4;
    State = CpuState::Fetch;
    MicroOp = 0;
    break;
  }
  }
}

} // namespace Aurelia::Cpu
