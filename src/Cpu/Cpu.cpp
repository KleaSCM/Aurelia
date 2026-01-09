/**
 * CPU Implementation (Pipeline FSM).
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#include "Cpu/Cpu.hpp"
#include "Core/BitManip.hpp"
#include "Cpu/Alu.hpp"
#include "Cpu/Decoder.hpp"
#include <cstring>

namespace Aurelia::Cpu {

Cpu::Cpu() { m_GPR.fill(0); }

void Cpu::ConnectBus(Bus::Bus *bus) { m_Bus = bus; }

void Cpu::Reset(Core::Address startAddress) {
  m_PC = startAddress;
  m_State = CpuState::Fetch;
  m_Flags = {};
  m_GPR.fill(0);
  m_MicroOp = 0;
  m_Halted = false;
}

Core::Word Cpu::GetRegister(Register reg) const {
  return m_GPR[static_cast<std::size_t>(reg)];
}

void Cpu::SetRegister(Register reg, Core::Word value) {
  m_GPR[static_cast<std::size_t>(reg)] = value;
}

Core::Address Cpu::GetPC() const { return m_PC; }

void Cpu::SetPC(Core::Address value) { m_PC = value; }

const Flags &Cpu::GetFlags() const { return m_Flags; }

void Cpu::OnTick() {
  if (!m_Bus || m_Halted)
    return;

  switch (m_State) {
  case CpuState::Fetch: {
    if (m_MicroOp == 0) {
      // Assert Read on PC
      m_Bus->SetAddress(m_PC);
      m_Bus->SetControl(Bus::ControlSignal::Read, true);
      m_Bus->SetControl(Bus::ControlSignal::Write, false);
      m_MicroOp = 1;
    } else {
      // Monitor Bus Wait Signal
      auto busState = m_Bus->GetState();
      if (!Core::CheckBit(busState.Control, 3)) { // Bit 3 = WAIT
        // Memory Ready. Latches Instruction
        std::uint32_t rawInstr = static_cast<std::uint32_t>(busState.DataBus);
        m_CurrentInstr = Decoder::Decode(rawInstr);

        // Clear Bus Request
        m_Bus->SetControl(Bus::ControlSignal::Read, false);

        m_State = CpuState::Decode;
        m_MicroOp = 0;
      }
    }
    break;
  }

  case CpuState::Decode: {
    // Read Operands
    if (m_CurrentInstr.Type == InstrType::Register) {
      m_OpA = GetRegister(m_CurrentInstr.Rn);
      m_OpB = GetRegister(m_CurrentInstr.Rm);
    } else if (m_CurrentInstr.Type == InstrType::Immediate) {
      m_OpA = GetRegister(
          m_CurrentInstr.Rn); // Base for LDR/STR, or ignored for MOV
      m_OpB = m_CurrentInstr.Immediate;
    } else if (m_CurrentInstr.Type == InstrType::Branch) {
      // Branch Offset
      m_OpB = m_CurrentInstr.Immediate;
    }

    m_State = CpuState::Execute;
    break;
  }

  case CpuState::Execute: {
    // Route to ALU
    AluOp aluOp = AluOp::ADD; // Default

    switch (m_CurrentInstr.Op) {
    case Opcode::ADD:
      aluOp = AluOp::ADD;
      break;
    case Opcode::SUB:
      aluOp = AluOp::SUB;
      break;
    case Opcode::AND:
      aluOp = AluOp::AND;
      break;
    case Opcode::OR:
      aluOp = AluOp::OR;
      break;
    case Opcode::XOR:
      aluOp = AluOp::XOR;
      break;
    case Opcode::LSL:
      aluOp = AluOp::LSL;
      break;
    case Opcode::LSR:
      aluOp = AluOp::LSR;
      break;
    case Opcode::MOV:
      // MOV is effectively OPA=0 + OPB
      m_OpA = 0;
      aluOp = AluOp::ADD;
      break;
    case Opcode::Halt:
      // HALT instruction - stop execution
      m_Halted = true;
      return;
    default:
      break;
    }

    if (m_CurrentInstr.Type == InstrType::Branch) {
      // Handle Branch Logic
      bool takeBranch = false;
      if (m_CurrentInstr.Op == Opcode::B) {
        takeBranch = true;
      } else if (m_CurrentInstr.Op == Opcode::BEQ && m_Flags.Z) {
        takeBranch = true;
      } else if (m_CurrentInstr.Op == Opcode::BNE && !m_Flags.Z) {
        takeBranch = true;
      }

      if (takeBranch) {
        m_PC += m_OpB;             // Relative Branch
        m_State = CpuState::Fetch; // Flush pipeline (simplification: just go to
                                   // fetch)
        m_MicroOp = 0;
        return;
      }
    } else if (m_CurrentInstr.Op == Opcode::LDR ||
               m_CurrentInstr.Op == Opcode::STR) {
      // Calculate Address: Rn + Imm
      m_AluResult = m_OpA + m_OpB; // Simple address calc
      m_State = CpuState::Memory;
      m_MicroOp = 0;
      return;
    } else {
      // ALU Operation
      auto res = Alu::Execute(aluOp, m_OpA, m_OpB, m_Flags);
      m_AluResult = res.Result;
      m_Flags = res.NewFlags;
    }

    m_State = CpuState::WriteBack;
    break;
  }

  case CpuState::Memory: {
    if (m_MicroOp == 0) {
      if (m_CurrentInstr.Op == Opcode::LDR) {
        m_Bus->SetAddress(m_AluResult);
        m_Bus->SetControl(Bus::ControlSignal::Read, true);
        m_Bus->SetControl(Bus::ControlSignal::Write, false);
      } else if (m_CurrentInstr.Op == Opcode::STR) {
        m_Bus->SetAddress(m_AluResult);
        m_Bus->SetData(GetRegister(m_CurrentInstr.Rd)); // Store Value
        m_Bus->SetControl(Bus::ControlSignal::Write, true);
        m_Bus->SetControl(Bus::ControlSignal::Read, false);
      }
      m_MicroOp = 1;
    } else {
      auto busState = m_Bus->GetState();
      if (!Core::CheckBit(busState.Control, 3)) { // Wait cleared
        if (m_CurrentInstr.Op == Opcode::LDR) {
          m_MemData = busState.DataBus;
          m_Bus->SetControl(Bus::ControlSignal::Read, false);
        } else {
          m_Bus->SetControl(Bus::ControlSignal::Write, false);
        }
        m_State = CpuState::WriteBack;
        m_MicroOp = 0;
      }
    }
    break;
  }

  case CpuState::WriteBack: {
    if (m_CurrentInstr.Op == Opcode::LDR) {
      SetRegister(m_CurrentInstr.Rd, m_MemData);
    } else if (m_CurrentInstr.Op != Opcode::STR &&
               m_CurrentInstr.Type != InstrType::Branch) {
      // ALU Result Writeback
      SetRegister(m_CurrentInstr.Rd, m_AluResult);
    }

    // Increment PC (if not branched)
    m_PC += 4;
    m_State = CpuState::Fetch;
    m_MicroOp = 0;
    break;
  }
  }
}

} // namespace Aurelia::Cpu
