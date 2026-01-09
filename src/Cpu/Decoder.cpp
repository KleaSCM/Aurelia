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
  // [31:24] Opcode
  Core::Byte opByte = static_cast<Core::Byte>((rawInstr >> 24) & 0xFF);
  instr.Op = static_cast<Opcode>(opByte);

  // [23:20] Rd
  instr.Rd = static_cast<Register>((rawInstr >> 20) & 0x0F);

  // [19:16] Rn
  instr.Rn = static_cast<Register>((rawInstr >> 16) & 0x0F);

  // [15:12] Rm
  instr.Rm = static_cast<Register>((rawInstr >> 12) & 0x0F);

  // [11: 0] Immediate (12 bits)
  instr.Immediate = rawInstr & 0xFFF;

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
