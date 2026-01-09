/**
 * CPU Definitions.
 *
 * Architecture definitions for the Aurelia CPU.
 * 16 General Purpose Registers (64-bit).
 * Standard Status Flags.
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#pragma once

#include <cstdint>

namespace Aurelia::Cpu {

// NOTE (KleaSCM) 16 General Purpose Registers + special aliases.
enum class Register : std::uint8_t {
  R0,
  R1,
  R2,
  R3,
  R4,
  R5,
  R6,
  R7,
  R8,
  R9,
  R10,
  R11,
  R12,
  R13,
  R14,
  R15,
  SP = R14, // R14 alias
  LR = R15, // R15 alias (Link Register)
  Count = 16
};

// NOTE (KleaSCM) Architectural Status Flags.
struct Flags {
  bool Z = false; // Zero
  bool N = false; // Negative
  bool C = false; // Carry
  bool V = false; // Overflow

  void Reset() { Z = N = C = V = false; }
};

} // namespace Aurelia::Cpu
