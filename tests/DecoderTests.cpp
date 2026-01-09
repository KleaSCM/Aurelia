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
  // Opcode ADD (0x01)
  // Rd (R1) = 1
  // Rn (R2) = 2
  // Rm (R3) = 3
  // 0x01 1 2 3 000
  std::uint32_t raw = 0x01123000;

  Instruction instr = Decoder::Decode(raw);

  CHECK(instr.Op == Opcode::ADD);
  CHECK(instr.Rd == Register::R1);
  CHECK(instr.Rn == Register::R2);
  CHECK(instr.Rm == Register::R3);
  CHECK(instr.Type == InstrType::Register);
}

TEST_CASE("Decoder - DecodeImmediateMove") {
  // MOV R5, #255
  // Opcode MOV (0x20)
  // Rd (R5) = 5
  // Immediate = 0xFF
  // 0x20 5 0 0 0FF
  std::uint32_t raw = 0x205000FF;

  Instruction instr = Decoder::Decode(raw);

  CHECK(instr.Op == Opcode::MOV);
  CHECK(instr.Rd == Register::R5);
  CHECK(instr.Immediate == 255);
  CHECK(instr.Type == InstrType::Immediate);
}

TEST_CASE("Decoder - DecodeBranch") {
  // B 0xABC
  // Opcode B (0x30)
  // Imm = 0xABC
  // 0x30 0 0 0 ABC
  std::uint32_t raw = 0x30000ABC;

  Instruction instr = Decoder::Decode(raw);

  CHECK(instr.Op == Opcode::B);
  CHECK(instr.Immediate == 0xABC);
  CHECK(instr.Type == InstrType::Branch);
}
