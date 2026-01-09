/**
 * Decoder Implementation.
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#include "Cpu/Decoder.hpp"

namespace Aurelia::Cpu {

Instruction Decoder::Decode(std::uint32_t rawInstr) {
  Instruction instr = {};

  // Extract Fields
  // [31:26] Opcode (6 bits)
  Core::Byte opByte = static_cast<Core::Byte>((rawInstr >> 26) & 0x3F);
  instr.Op = static_cast<Opcode>(opByte);

  // [25:21] Rd (5 bits)
  instr.Rd = static_cast<Register>((rawInstr >> 21) & 0x1F);

  // [20:16] Rn (5 bits)
  instr.Rn = static_cast<Register>((rawInstr >> 16) & 0x1F);

  // [15:11] Rm (5 bits)
  instr.Rm = static_cast<Register>((rawInstr >> 11) & 0x1F);

  // [10: 0] Immediate (11 bits)
  instr.Immediate = rawInstr & 0x7FF;

  // Determine Type based on Opcode
  switch (instr.Op) {
  case Opcode::LDR:
  case Opcode::STR:
  case Opcode::MOV:
    instr.Type = InstrType::Immediate; // Can be reg or imm, simplifying to Imm
                                       // for now logic
    break;
  case Opcode::B:
  case Opcode::BEQ:
  case Opcode::BNE:
    instr.Type = InstrType::Branch;
    // Branch offsets are signed 24-bit in real ARM, here simplistic 12-bit
    break;
  default:
    instr.Type = InstrType::Register;
    break;
  }

  return instr;
}

} // namespace Aurelia::Cpu
