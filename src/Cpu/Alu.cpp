/**
 * ALU Implementation.
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#include "Cpu/Alu.hpp"
#include "Core/BitManip.hpp"

namespace Aurelia::Cpu {

AluResult Alu::Execute(AluOp op, Core::Word a, Core::Word b,
                       const Flags &currentFlags) {
  AluResult res = {0, {false, false, false, false}};

  // Helper for Flag Calc
  auto setZ = [&](Core::Word val) { res.NewFlags.Z = (val == 0); };
  auto setN = [&](Core::Word val) { res.NewFlags.N = Core::CheckBit(val, 63); };

  switch (op) {
  case AluOp::ADD: {
    res.Result = a + b;
    setZ(res.Result);
    setN(res.Result);
    // C: Unsigned Overflow
    res.NewFlags.C = (res.Result < a);
    // V: Signed Overflow (Pos+Pos=Neg or Neg+Neg=Pos)
    bool aNeg = Core::CheckBit(a, 63);
    bool bNeg = Core::CheckBit(b, 63);
    bool rNeg = Core::CheckBit(res.Result, 63);
    res.NewFlags.V = (aNeg == bNeg) && (aNeg != rNeg);
    break;
  }
  case AluOp::SUB: {
    res.Result = a - b;
    setZ(res.Result);
    setN(res.Result);
    // C: Borrow (x86 style: C=1 if A < B).
    res.NewFlags.C = (a < b);
    // V: Signed Overflow (Pos-Neg=Neg or Neg-Pos=Pos)
    // A - B = R => A = R + B
    bool aNeg = Core::CheckBit(a, 63);
    bool bNeg = Core::CheckBit(b, 63);
    bool rNeg = Core::CheckBit(res.Result, 63);
    // Overflow (V) = (Operand signs differ) AND (Result sign differs from OpA)
    // For SUB logic: A - B is A + (-B).
    res.NewFlags.V = (aNeg != bNeg) && (aNeg != rNeg);
    res.NewFlags.V = (aNeg != bNeg) && (rNeg != aNeg);
    break;
  }
  case AluOp::AND:
    res.Result = a & b;
    setZ(res.Result);
    setN(res.Result);
    res.NewFlags.C = currentFlags.C; // Preserve C
    res.NewFlags.V = false;          // Cleared
    break;

  case AluOp::OR:
    res.Result = a | b;
    setZ(res.Result);
    setN(res.Result);
    res.NewFlags.C = currentFlags.C;
    res.NewFlags.V = false;
    break;

  case AluOp::XOR:
    res.Result = a ^ b;
    setZ(res.Result);
    setN(res.Result);
    res.NewFlags.C = currentFlags.C;
    res.NewFlags.V = false;
    break;

  case AluOp::LSL: {
    // Logical Shift Left
    // Shift amount is B (lowest 6 bits usually, let's limit to < 64)
    unsigned int shift = b & 0x3F;
    if (shift == 0) {
      res.Result = a;
      res.NewFlags.C = currentFlags.C;
    } else {
      res.Result = a << shift;
      // C is the last bit shifted out (bit 64 - shift)
      res.NewFlags.C = Core::CheckBit(a, 64 - shift);
    }
    setZ(res.Result);
    setN(res.Result);
    res.NewFlags.V = false; // Shifts don't set Overflow generally in ARM
    break;
  }

  case AluOp::LSR: {
    // Logical Shift Right
    unsigned int shift = b & 0x3F;
    if (shift == 0) {
      res.Result = a;
      res.NewFlags.C = currentFlags.C;
    } else {
      res.Result = a >> shift;
      // C is the last bit shifted out (bit shift - 1)
      res.NewFlags.C = Core::CheckBit(a, shift - 1);
    }
    setZ(res.Result);
    setN(res.Result);
    res.NewFlags.V = false;
    break;
  }

  case AluOp::ASR: {
    // Arithmetic Shift Right (Preserve Sign)
    unsigned int shift = b & 0x3F;
    // C++ signed right shift is implementation defined, mostly arithmetic.
    // We enforce it by casting to signed.
    std::int64_t signedA = static_cast<std::int64_t>(a);
    if (shift == 0) {
      res.Result = a;
      res.NewFlags.C = currentFlags.C;
    } else {
      res.Result = static_cast<Core::Word>(signedA >> shift);
      res.NewFlags.C = Core::CheckBit(a, shift - 1);
    }
    setZ(res.Result);
    setN(res.Result);
    res.NewFlags.V = false;
    break;
  }

  default:
    break;
  }

  return res;
}

} // namespace Aurelia::Cpu
