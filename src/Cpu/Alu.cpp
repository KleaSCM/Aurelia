/**
 * ALU Implementation.
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#include "Cpu/Alu.hpp"
#include "Core/BitManip.hpp"

namespace Aurelia::Cpu {

AluResult Alu::Execute(AluOp Op, Core::Word A, Core::Word B,
                       const Flags &CurrentFlags) {
  AluResult Res = {0, {false, false, false, false}};

  // Helper for Flag Calc
  auto SetZ = [&](Core::Word Val) { Res.NewFlags.Z = (Val == 0); };
  auto SetN = [&](Core::Word Val) { Res.NewFlags.N = Core::CheckBit(Val, 63); };

  switch (Op) {
  case AluOp::ADD: {
    Res.Result = A + B;
    SetZ(Res.Result);
    SetN(Res.Result);

    /**
     * CARRY FLAG (C)
     * Unsigned Overflow: Result wrap-around.
     * If (A + B) < A, then an overflow occurred.
     */
    Res.NewFlags.C = (Res.Result < A);

    /**
     * OVERFLOW FLAG (V)
     * Signed Overflow.
     * Occurs when adding two numbers of same sign produces a result of
     * different sign. Logic: V = (A_sign == B_sign) && (A_sign != Result_sign)
     */
    bool aNeg = Core::CheckBit(A, 63);
    bool bNeg = Core::CheckBit(B, 63);
    bool rNeg = Core::CheckBit(Res.Result, 63);
    Res.NewFlags.V = (aNeg == bNeg) && (aNeg != rNeg);
    break;
  }
  case AluOp::SUB: {
    Res.Result = A - B;
    SetZ(Res.Result);
    SetN(Res.Result);

    /**
     * BORROW FLAG (C)
     * x86 Style Borrow: C=1 if A < B (unsigned check).
     */
    Res.NewFlags.C = (A < B);

    /**
     * OVERFLOW FLAG (V)
     * Signed Overflow for Subtraction.
     * Logic: A - B is equivalent to A + (-B).
     * V = (A_sign != B_sign) && (A_sign != Result_sign)
     * (Subtracting a negative from a positive -> positive + positive)
     */
    bool aNeg = Core::CheckBit(A, 63);
    bool bNeg = Core::CheckBit(B, 63);
    bool rNeg = Core::CheckBit(Res.Result, 63);
    Res.NewFlags.V = (aNeg != bNeg) && (aNeg != rNeg);
    break;
  }
  case AluOp::AND:
    Res.Result = A & B;
    SetZ(Res.Result);
    SetN(Res.Result);
    Res.NewFlags.C = CurrentFlags.C; // Preserve C
    Res.NewFlags.V = false;          // Cleared
    break;

  case AluOp::OR:
    Res.Result = A | B;
    SetZ(Res.Result);
    SetN(Res.Result);
    Res.NewFlags.C = CurrentFlags.C;
    Res.NewFlags.V = false;
    break;

  case AluOp::XOR:
    Res.Result = A ^ B;
    SetZ(Res.Result);
    SetN(Res.Result);
    Res.NewFlags.C = CurrentFlags.C;
    Res.NewFlags.V = false;
    break;

  case AluOp::LSL: {
    /**
     * LOGICAL SHIFT LEFT
     * Shifts bits to higher significance. Empty LSBs filled with 0.
     * Shift amount limited to [0, 63] via mask 0x3F.
     */
    unsigned int shift = B & 0x3F;
    if (shift == 0) {
      Res.Result = A;
      Res.NewFlags.C = CurrentFlags.C;
    } else {
      Res.Result = A << shift;
      // C is the last bit shifted out (bit 64 - shift)
      Res.NewFlags.C = Core::CheckBit(A, 64 - shift);
    }
    SetZ(Res.Result);
    SetN(Res.Result);
    Res.NewFlags.V = false;
    break;
  }

  case AluOp::LSR: {
    /**
     * LOGICAL SHIFT RIGHT
     * Shifts bits to lower significance. Empty MSBs filled with 0.
     */
    unsigned int shift = B & 0x3F;
    if (shift == 0) {
      Res.Result = A;
      Res.NewFlags.C = CurrentFlags.C;
    } else {
      Res.Result = A >> shift;
      // C is the last bit shifted out (bit shift - 1)
      Res.NewFlags.C = Core::CheckBit(A, shift - 1);
    }
    SetZ(Res.Result);
    SetN(Res.Result);
    Res.NewFlags.V = false;
    break;
  }

  case AluOp::ASR: {
    /**
     * ARITHMETIC SHIFT RIGHT
     * Shifts bits to lower significance, preserving the Sign Bit (MSB).
     * Implemented by casting to signed int64_t before shifting.
     */
    unsigned int shift = B & 0x3F;
    std::int64_t signedA = static_cast<std::int64_t>(A);

    if (shift == 0) {
      Res.Result = A;
      Res.NewFlags.C = CurrentFlags.C;
    } else {
      Res.Result = static_cast<Core::Word>(signedA >> shift);
      Res.NewFlags.C = Core::CheckBit(A, shift - 1);
    }
    SetZ(Res.Result);
    SetN(Res.Result);
    Res.NewFlags.V = false;
    break;
  }

  default:
    break;
  }

  return Res;
}

} // namespace Aurelia::Cpu
