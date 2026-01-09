/**
 * Instruction Set Architecture (ISA) Definitions.
 *
 * Defines the Instruction Format and Opcode mappings.
 *
 * Instruction Format (32-bit):
 * [31:24] Opcode (8 bits)
 * [23:20] Rd (Dest Register)
 * [19:16] Rn (Source 1)
 * [15:12] Rm (Source 2)
 * [11: 0] Immediate / Reserved
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#pragma once

#include "Core/Types.hpp"
#include "Cpu/CpuDefs.hpp"

namespace Aurelia::Cpu {

enum class Opcode : Core::Byte {
  NOP = 0x00,

  // Arithmetic (ALU)
  ADD = 0x01,
  SUB = 0x02,
  AND = 0x03,
  OR = 0x04,
  XOR = 0x05,
  LSL = 0x06,
  LSR = 0x07,

  // Memory
  LDR = 0x10, // Load Register
  STR = 0x11, // Store Register

  // Control Flow
  MOV = 0x20, // Move Register/Immediate
  B = 0x30,   // Branch (Unconditional)
  BEQ = 0x31, // Branch Equal (Z=1)
  BNE = 0x32, // Branch Not Equal (Z=0)

  Halt = 0xFF
};

enum class InstrType {
  Register,  // Uses Rd, Rn, Rm
  Immediate, // Uses Rd, Imm
  Branch     // Uses Offset (Imm)
};

struct Instruction {
  Opcode Op;
  Register Rd;
  Register Rn;
  Register Rm;
  Core::Word Immediate; // Expanded to 64-bit for convenience
  InstrType Type;
};

} // namespace Aurelia::Cpu
