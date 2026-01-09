/**
 * Decoder Tests.
 *
 * Verifies correct bit-field extraction from raw instruction words.
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#include "Cpu/Decoder.hpp"
#include <catch2/catch_test_macros.hpp>

using namespace Aurelia::Cpu;
using namespace Aurelia::Core;

TEST_CASE("Decoder - DecodeRegisterArithmetic") {
  // ADD R1, R2, R3
  // Opcode ADD (0x01) << 26
  // Rd (R1) = 1 << 21
  // Rn (R2) = 2 << 16
  // Rm (R3) = 3 << 11
  // Imm = 0
  // Value: 0x04000000 | 0x00200000 | 0x00020000 | 0x00001800
  std::uint32_t raw = 0x04221800;

  Instruction instr = Decoder::Decode(raw);

  CHECK(instr.Op == Opcode::ADD);
  CHECK(instr.Rd == Register::R1);
  CHECK(instr.Rn == Register::R2);
  CHECK(instr.Rm == Register::R3);
  CHECK(instr.Type == InstrType::Register);
}

TEST_CASE("Decoder - DecodeImmediateMove") {
  // MOV R5, #255
  // Opcode MOV (0x20) << 26 = 0x80000000
  // Rd (R5) = 5 << 21 = 0x00A00000
  // Immediate = 0xFF
  std::uint32_t raw = 0x80A000FF;

  Instruction instr = Decoder::Decode(raw);

  CHECK(instr.Op == Opcode::MOV);
  CHECK(instr.Rd == Register::R5);
  CHECK(instr.Immediate == 255);
  CHECK(instr.Type == InstrType::Immediate);
}

TEST_CASE("Decoder - DecodeBranch") {
  // B 0x2BC (Fits in 11 bits)
  // Opcode B (0x30) << 26 = 0xC0000000
  // Imm = 0x2BC
  std::uint32_t raw = 0xC00002BC;

  Instruction instr = Decoder::Decode(raw);

  CHECK(instr.Op == Opcode::B);
  CHECK(instr.Immediate == 0x2BC);
  CHECK(instr.Type == InstrType::Branch);
}
