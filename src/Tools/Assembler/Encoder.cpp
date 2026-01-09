/**
 * Assembler Encoder Implementation.
 *
 * Generates 32-bit machine code from ParsedInstructions with strict
 * per-opcode validation. This implementation enforces correct operand
 * shapes and ranges for every instruction, rejecting malformed inputs
 * that would otherwise produce semantically incorrect binaries.
 *
 * INSTRUCTION FORMAT (32-bit Fixed Width):
 * ┌─────────┬─────┬─────┬─────┬────────────┐
 * │ Opcode  │ Rd  │ Rn  │ Rm  │ Immediate  │
 * │ [31:26] │[25:21]│[20:16]│[15:11]│ [10:0]    │
 * │ 6 bits  │5 bits│5 bits│5 bits│ 11 bits   │
 * └─────────┴─────┴─────┴─────┴────────────┘
 *
 * ENCODING GUARANTEES:
 * - Opcode enum values MUST match ISA binary encoding (see InstructionDefs.hpp)
 * - All operand shapes validated against opcode-specific requirements
 * - Immediate values range-checked before encoding
 * - Invalid operand combinations rejected with detailed error messages
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#include "Tools/Assembler/Encoder.hpp"
#include "Cpu/InstructionDefs.hpp"
#include <limits>

namespace Aurelia::Tools::Assembler {

Encoder::Encoder(const std::vector<ParsedInstruction> &instructions)
    : m_Instructions(instructions) {}

bool Encoder::Encode() {
  m_Binary.clear();

  if (m_Instructions.size() > 0) {
    m_Binary.reserve(m_Instructions.size() * 4);
  }

  for (const auto &instr : m_Instructions) {
    std::uint32_t encoded = EncodeInstruction(instr);
    if (m_HasError) {
      return false;
    }

    // Little-endian byte emission (LSB first)
    m_Binary.push_back(static_cast<std::uint8_t>(encoded & 0xFF));
    m_Binary.push_back(static_cast<std::uint8_t>((encoded >> 8) & 0xFF));
    m_Binary.push_back(static_cast<std::uint8_t>((encoded >> 16) & 0xFF));
    m_Binary.push_back(static_cast<std::uint8_t>((encoded >> 24) & 0xFF));
  }

  return true;
}

std::uint32_t Encoder::EncodeInstruction(const ParsedInstruction &instr) {
  /**
   * OPCODE-SPECIFIC ENCODING STRATEGY
   *
   * Each opcode has a specific operand format. We validate the actual
   * operand count and types against the expected pattern, then encode
   * only if the instruction is well-formed.
   *
   * NOTE (KleaSCM) Cpu::Opcode enum values are defined to match ISA
   * binary encoding exactly (see InstructionDefs.hpp).
   */

  std::uint32_t opcode = static_cast<std::uint32_t>(instr.Op);
  std::uint32_t rd = 0;
  std::uint32_t rn = 0;
  std::uint32_t rm = 0;
  std::uint32_t imm = 0;

  // Per-opcode encoding with strict validation
  switch (instr.Op) {
  case Cpu::Opcode::NOP:
  case Cpu::Opcode::Halt:
    /**
     * 0-Operand Instructions (Control)
     * Format: Op (all register/immediate fields zero)
     */
    if (!instr.Operands.empty()) {
      Error(instr, instr.Mnemonic + " takes no operands");
      return 0;
    }
    break;

  case Cpu::Opcode::B:
  case Cpu::Opcode::BEQ:
  case Cpu::Opcode::BNE:
    /**
     * Branch Instructions
     * Format: Op #Offset
     *
     * ENCODING REQUIREMENTS:
     * - Exactly 1 operand (PC-relative offset)
     * - Operand must be Immediate (Resolver converts labels)
     * - Offset must fit in signed 11-bit range [-1024, +1023]
     * - Offset is in BYTES (Resolver calculates: Target - (PC+4))
     *
     * NOTE (KleaSCM) With 11-bit signed offset and 4-byte instructions:
     * Maximum forward jump: +1023 bytes = 255 instructions
     * Maximum backward jump: -1024 bytes = 256 instructions
     */
    if (instr.Operands.size() != 1) {
      Error(instr, instr.Mnemonic + " requires exactly 1 operand (offset)");
      return 0;
    }

    if (instr.Operands[0].Type != OperandType::Immediate) {
      Error(instr, instr.Mnemonic + " operand must be immediate offset (labels "
                                    "resolved by Resolver)");
      return 0;
    }

    if (!std::holds_alternative<ImmediateOperand>(instr.Operands[0].Value)) {
      Error(instr, "Internal error: Immediate operand has wrong variant type");
      return 0;
    }

    {
      std::int64_t offset = static_cast<std::int64_t>(
          std::get<ImmediateOperand>(instr.Operands[0].Value).Value);

      // Validate signed 11-bit range
      if (offset < -1024 || offset > 1023) {
        Error(instr, "Branch offset out of range: " + std::to_string(offset) +
                         " (must be in [-1024, +1023])");
        return 0;
      }

      // Store as unsigned for two's complement representation
      imm = static_cast<std::uint32_t>(offset) & 0x7FF;
    }
    break;

  case Cpu::Opcode::MOV:
    /**
     * Move Instruction
     * Format: MOV Rd, Src
     * Where Src can be:
     * - Register (Rm)
     * - Immediate (#Imm)
     *
     * ENCODING:
     * - MOV Rd, Rm  → Rd and Rm fields used, Rn=0
     * - MOV Rd, #Imm → Rd and Immediate fields used, Rn=0, Rm=0
     */
    if (instr.Operands.size() != 2) {
      Error(instr, "MOV requires exactly 2 operands (Rd, Src)");
      return 0;
    }

    // Destination must be register
    if (instr.Operands[0].Type != OperandType::Register) {
      Error(instr, "MOV destination must be a register");
      return 0;
    }

    if (!std::holds_alternative<RegisterOperand>(instr.Operands[0].Value)) {
      Error(instr, "Internal error: Register operand has wrong variant type");
      return 0;
    }

    rd = static_cast<std::uint32_t>(
        std::get<RegisterOperand>(instr.Operands[0].Value).RegIndex);

    // Source can be register or immediate
    if (instr.Operands[1].Type == OperandType::Register) {
      if (!std::holds_alternative<RegisterOperand>(instr.Operands[1].Value)) {
        Error(instr, "Internal error: Register operand has wrong variant type");
        return 0;
      }
      rm = static_cast<std::uint32_t>(
          std::get<RegisterOperand>(instr.Operands[1].Value).RegIndex);
    } else if (instr.Operands[1].Type == OperandType::Immediate) {
      if (!std::holds_alternative<ImmediateOperand>(instr.Operands[1].Value)) {
        Error(instr,
              "Internal error: Immediate operand has wrong variant type");
        return 0;
      }

      std::uint64_t val =
          std::get<ImmediateOperand>(instr.Operands[1].Value).Value;

      // Validate 11-bit unsigned range [0, 2047]
      if (val > 2047) {
        Error(instr, "MOV immediate out of range: " + std::to_string(val) +
                         " (must be in [0, 2047])");
        return 0;
      }

      imm = static_cast<std::uint32_t>(val);
    } else {
      Error(instr, "MOV source must be register or immediate");
      return 0;
    }
    break;

  case Cpu::Opcode::CMP:
    /**
     * Compare Instruction
     * Format: CMP Rn, Src
     * Where Src can be:
     * - Register (Rm)
     * - Immediate (#Imm)
     *
     * ENCODING:
     * - CMP Rn, Rm  → Rn and Rm fields used, Rd=0
     * - CMP Rn, #Imm → Rn and Immediate fields used, Rd=0, Rm=0
     *
     * NOTE (KleaSCM) CMP updates status flags only (no destination register).
     */
    if (instr.Operands.size() != 2) {
      Error(instr, "CMP requires exactly 2 operands (Rn, Src)");
      return 0;
    }

    // First operand must be register (mapped to Rn, NOT Rd)
    if (instr.Operands[0].Type != OperandType::Register) {
      Error(instr, "CMP first operand must be a register");
      return 0;
    }

    if (!std::holds_alternative<RegisterOperand>(instr.Operands[0].Value)) {
      Error(instr, "Internal error: Register operand has wrong variant type");
      return 0;
    }

    rn = static_cast<std::uint32_t>(
        std::get<RegisterOperand>(instr.Operands[0].Value).RegIndex);

    // Second operand can be register or immediate
    if (instr.Operands[1].Type == OperandType::Register) {
      if (!std::holds_alternative<RegisterOperand>(instr.Operands[1].Value)) {
        Error(instr, "Internal error: Register operand has wrong variant type");
        return 0;
      }
      rm = static_cast<std::uint32_t>(
          std::get<RegisterOperand>(instr.Operands[1].Value).RegIndex);
    } else if (instr.Operands[1].Type == OperandType::Immediate) {
      if (!std::holds_alternative<ImmediateOperand>(instr.Operands[1].Value)) {
        Error(instr,
              "Internal error: Immediate operand has wrong variant type");
        return 0;
      }

      std::uint64_t val =
          std::get<ImmediateOperand>(instr.Operands[1].Value).Value;

      if (val > 2047) {
        Error(instr, "CMP immediate out of range: " + std::to_string(val) +
                         " (must be in [0, 2047])");
        return 0;
      }

      imm = static_cast<std::uint32_t>(val);
    } else {
      Error(instr, "CMP second operand must be register or immediate");
      return 0;
    }
    break;

  case Cpu::Opcode::ADD:
  case Cpu::Opcode::SUB:
  case Cpu::Opcode::AND:
  case Cpu::Opcode::OR:
  case Cpu::Opcode::XOR:
  case Cpu::Opcode::LSL:
  case Cpu::Opcode::LSR:
  case Cpu::Opcode::ASR:
    /**
     * 3-Operand Arithmetic/Logical Instructions
     * Format: Op Rd, Rn, Src
     * Where Src can be:
     * - Register (Rm)
     * - Immediate (#Imm)
     *
     * ENCODING:
     * - Op Rd, Rn, Rm  → All three register fields used
     * - Op Rd, Rn, #Imm → Rd, Rn, and Immediate fields used, Rm=0
     */
    if (instr.Operands.size() != 3) {
      Error(instr,
            instr.Mnemonic + " requires exactly 3 operands (Rd, Rn, Src)");
      return 0;
    }

    // Destination must be register
    if (instr.Operands[0].Type != OperandType::Register) {
      Error(instr, instr.Mnemonic + " destination must be a register");
      return 0;
    }

    if (!std::holds_alternative<RegisterOperand>(instr.Operands[0].Value)) {
      Error(instr, "Internal error: Register operand has wrong variant type");
      return 0;
    }

    rd = static_cast<std::uint32_t>(
        std::get<RegisterOperand>(instr.Operands[0].Value).RegIndex);

    // First source must be register
    if (instr.Operands[1].Type != OperandType::Register) {
      Error(instr, instr.Mnemonic + " first source must be a register");
      return 0;
    }

    if (!std::holds_alternative<RegisterOperand>(instr.Operands[1].Value)) {
      Error(instr, "Internal error: Register operand has wrong variant type");
      return 0;
    }

    rn = static_cast<std::uint32_t>(
        std::get<RegisterOperand>(instr.Operands[1].Value).RegIndex);

    // Second source can be register or immediate
    if (instr.Operands[2].Type == OperandType::Register) {
      if (!std::holds_alternative<RegisterOperand>(instr.Operands[2].Value)) {
        Error(instr, "Internal error: Register operand has wrong variant type");
        return 0;
      }
      rm = static_cast<std::uint32_t>(
          std::get<RegisterOperand>(instr.Operands[2].Value).RegIndex);
    } else if (instr.Operands[2].Type == OperandType::Immediate) {
      if (!std::holds_alternative<ImmediateOperand>(instr.Operands[2].Value)) {
        Error(instr,
              "Internal error: Immediate operand has wrong variant type");
        return 0;
      }

      std::uint64_t val =
          std::get<ImmediateOperand>(instr.Operands[2].Value).Value;

      if (val > 2047) {
        Error(instr, instr.Mnemonic + " immediate out of range: " +
                         std::to_string(val) + " (must be in [0, 2047])");
        return 0;
      }

      imm = static_cast<std::uint32_t>(val);
    } else {
      Error(instr,
            instr.Mnemonic + " second source must be register or immediate");
      return 0;
    }
    break;

  case Cpu::Opcode::LDR:
  case Cpu::Opcode::STR:
    /**
     * Memory Access Instructions
     * Format: Op Rd, [Rn, #Offset]
     *
     * ENCODING:
     * - Rd: Data register (destination for LDR, source for STR)
     * - Rn: Base address register
     * - Immediate: Signed offset from base (11-bit signed range)
     *
     * ADDRESSING:
     * Effective Address = Rn + SignExtend(Offset)
     *
     * NOTE (KleaSCM) For STR, Rd semantically acts as source but
     * still encodes in the Rd field per ISA specification.
     */
    if (instr.Operands.size() != 2) {
      Error(instr, instr.Mnemonic +
                       " requires exactly 2 operands (Rd, [Rn, #Offset])");
      return 0;
    }

    // First operand must be register (data register)
    if (instr.Operands[0].Type != OperandType::Register) {
      Error(instr, instr.Mnemonic + " data operand must be a register");
      return 0;
    }

    if (!std::holds_alternative<RegisterOperand>(instr.Operands[0].Value)) {
      Error(instr, "Internal error: Register operand has wrong variant type");
      return 0;
    }

    rd = static_cast<std::uint32_t>(
        std::get<RegisterOperand>(instr.Operands[0].Value).RegIndex);

    // Second operand must be memory (bracket syntax)
    if (instr.Operands[1].Type != OperandType::Memory) {
      Error(instr, instr.Mnemonic +
                       " address operand must be memory syntax [Rn, #Offset]");
      return 0;
    }

    if (!std::holds_alternative<MemoryOperand>(instr.Operands[1].Value)) {
      Error(instr, "Internal error: Memory operand has wrong variant type");
      return 0;
    }

    {
      auto &mem = std::get<MemoryOperand>(instr.Operands[1].Value);
      rn = static_cast<std::uint32_t>(mem.BaseReg);

      // Validate signed 11-bit offset range
      if (mem.Offset < -1024 || mem.Offset > 1023) {
        Error(instr, instr.Mnemonic +
                         " offset out of range: " + std::to_string(mem.Offset) +
                         " (must be in [-1024, +1023])");
        return 0;
      }

      // Encode as unsigned two's complement
      imm = static_cast<std::uint32_t>(mem.Offset) & 0x7FF;
    }
    break;

  default:
    Error(instr, "Unknown or unimplemented opcode: " + instr.Mnemonic);
    return 0;
  }

  /**
   * FINAL ASSEMBLY
   *
   * Combine validated fields into 32-bit instruction word.
   * All fields are masked to their specified widths to guarantee correctness.
   */
  std::uint32_t word = 0;
  word |= (opcode & 0x3F) << 26; // Opcode: 6 bits [31:26]
  word |= (rd & 0x1F) << 21;     // Rd: 5 bits [25:21]
  word |= (rn & 0x1F) << 16;     // Rn: 5 bits [20:16]
  word |= (rm & 0x1F) << 11;     // Rm: 5 bits [15:11]
  word |= (imm & 0x7FF);         // Immediate: 11 bits [10:0]

  return word;
}

void Encoder::Error(const ParsedInstruction &instr,
                    const std::string &message) {
  if (m_HasError) {
    return; // Suppress cascading errors (fail-fast)
  }
  m_HasError = true;
  m_ErrorMessage =
      "[Line " + std::to_string(instr.Line) + "] Encoder: " + message;
}

} // namespace Aurelia::Tools::Assembler
