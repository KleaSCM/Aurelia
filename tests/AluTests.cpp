/**
 * ALU Tests.
 *
 * Verifies arithmetic correctness and flag generation.
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#include "Cpu/Alu.hpp"
#include <catch2/catch_test_macros.hpp>
#include <limits>

using namespace Aurelia::Cpu;
using namespace Aurelia::Core;

TEST_CASE("Alu - AddBasic") {
  Flags f = {0};
  AluResult res = Alu::Execute(AluOp::ADD, 10, 20, f);
  CHECK(res.Result == 30);
  CHECK_FALSE(res.NewFlags.Z);
  CHECK_FALSE(res.NewFlags.N);
  CHECK_FALSE(res.NewFlags.C);
  CHECK_FALSE(res.NewFlags.V);
}

TEST_CASE("Alu - AddZero") {
  Flags f = {0};
  AluResult res = Alu::Execute(AluOp::ADD, 0, 0, f);
  CHECK(res.Result == 0);
  CHECK(res.NewFlags.Z);
}

TEST_CASE("Alu - AddCarry") {
  Flags f = {0};
  Word max = std::numeric_limits<Word>::max();
  // Max + 1 = 0, Carry Set
  AluResult res = Alu::Execute(AluOp::ADD, max, 1, f);
  CHECK(res.Result == 0);
  CHECK(res.NewFlags.Z);
  CHECK(res.NewFlags.C);
}

TEST_CASE("Alu - SubBorrow") {
  Flags f = {0};
  // 5 - 10 = -5 (Two's complement) -> Borrow
  AluResult res = Alu::Execute(AluOp::SUB, 5, 10, f);

  // -5 in 64-bit: 0xFFFFFFFFFFFFFFFB
  CHECK(res.Result == 0xFFFFFFFFFFFFFFFB);
  CHECK(res.NewFlags.N); // Negative result
  CHECK(res.NewFlags.C); // Borrow
}

TEST_CASE("Alu - SignedOverflow") {
  Flags f = {0};
  // MaxPositive = 0x7FFFF...
  Word maxPos = 0x7FFFFFFFFFFFFFFF;
  // MaxPos + 1 = 0x8000... (Negative) -> Overflow
  AluResult res = Alu::Execute(AluOp::ADD, maxPos, 1, f);
  CHECK(res.Result == 0x8000000000000000);
  CHECK(res.NewFlags.N);
  CHECK(res.NewFlags.V);
}
