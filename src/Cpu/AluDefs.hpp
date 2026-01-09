/**
 * ALU Definitions.
 *
 * Defines the Arithmetic and Logic operations supported by the CPU.
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#pragma once

#include "Core/Types.hpp"
#include "Cpu/CpuDefs.hpp"

namespace Aurelia::Cpu {

enum class AluOp {
  ADD, // A + B
  SUB, // A - B
  AND, // A & B
  OR,  // A | B
  XOR, // A ^ B
  LSL, // Logical Shift Left
  LSR, // Logical Shift Right
  ASR, // Arithmetic Shift Right
  // Extended Ops
  ADC, // Add with Carry
  SBB  // Subtract with Borrow
};

struct AluResult {
  Core::Word Result;
  Flags NewFlags;
};

} // namespace Aurelia::Cpu
