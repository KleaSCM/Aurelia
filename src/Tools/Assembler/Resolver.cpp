/**
 * Assembler Resolver Implementation.
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#include "Tools/Assembler/Resolver.hpp"
#include <algorithm>
#include <cmath>

namespace Aurelia::Tools::Assembler {

using Address = Core::Address;

Resolver::Resolver(std::vector<ParsedInstruction> &instructions,
                   const std::vector<Parser::LabelDef> &labels)
    : m_Instructions(instructions), m_Labels(labels) {}

bool Resolver::Resolve() {
  BuildSymbolTable();
  if (m_HasError)
    return false;

  ResolveOperands();
  return !m_HasError;
}

void Resolver::BuildSymbolTable() {
  // Pass 1: Assign Addresses (4 bytes per instruction)
  for (const auto &label : m_Labels) {
    Address addr = static_cast<Address>(label.InstructionIndex * 4);

    if (m_SymbolTable.Contains(label.Name)) {
      m_ErrorMessage = "Duplicate Label Definition: " + label.Name;
      m_HasError = true;
      return;
    }

    m_SymbolTable.Define(label.Name, addr);
  }
}

void Resolver::ResolveOperands() {
  // Pass 2: Patch Operands
  for (size_t i = 0; i < m_Instructions.size(); ++i) {
    auto &instr = m_Instructions[i];
    Address currentAddr = static_cast<Address>(i * 4);

    for (auto &op : instr.Operands) {
      if (op.Type == OperandType::Label) {
        // Resolve Label
        auto labelOp = std::get<LabelOperand>(op.Value);
        std::string name = labelOp.Name;

        auto targetAddrOpt = m_SymbolTable.Resolve(name);
        if (!targetAddrOpt.has_value()) {
          Error(instr, "Undefined Symbol: " + name);
          return;
        }
        Address targetAddr = targetAddrOpt.value();

        // Branch Resolution (PC Relative)
        // B, BEQ, BNE, CMP? No CMP doesn't use label usually.
        // BL (if exists).
        // We assume Branch opcodes need PC-Relative offset.
        bool isBranch =
            (instr.Op == Cpu::Opcode::B || instr.Op == Cpu::Opcode::BEQ ||
             instr.Op == Cpu::Opcode::BNE);

        if (isBranch) {
          // PC is Current (Cpu adds OpB to current PC)
          // Offset = Target - PC
          std::int64_t diff = static_cast<std::int64_t>(targetAddr) -
                              static_cast<std::int64_t>(currentAddr);

          // Check Range (11-bit signed: -1024 to +1023)
          if (diff < -1024 || diff > 1023) {
            Error(instr,
                  "Branch target out of range (" + std::to_string(diff) + ")");
            return;
          }

          // Replace with Immediate
          op.Type = OperandType::Immediate;
          // Cast to uint64 for storage, Encoder will enforce bitmask
          op.Value = ImmediateOperand{static_cast<std::uint64_t>(diff)};
        } else {
          // Absolute address for non-branch instructions
          op.Type = OperandType::Immediate;
          op.Value = ImmediateOperand{targetAddr};
        }
      }
    }
  }
}

void Resolver::Error(const ParsedInstruction &instr,
                     const std::string &message) {
  if (m_HasError)
    return;
  m_HasError = true;
  m_ErrorMessage = "[Line " + std::to_string(instr.Line) + "] " + message;
}

} // namespace Aurelia::Tools::Assembler
