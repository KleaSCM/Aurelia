/**
 * Assembler Resolver Tests.
 *
 * Verifies the Two-Pass resolution logic.
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#include "Tools/Assembler/Lexer.hpp"
#include "Tools/Assembler/Parser.hpp"
#include "Tools/Assembler/Resolver.hpp"
#include <catch2/catch_test_macros.hpp>
#include <variant>

using namespace Aurelia::Tools::Assembler;
using namespace Aurelia::Cpu;

TEST_CASE("Resolver - Forward Branch") {
  // 0: B target
  // 1: NOP
  // 2: target: HALT
  std::string source = "B target\nNOP\ntarget:\nHALT";
  Lexer lexer(source);
  auto tokens = lexer.Tokenize();
  Parser parser(tokens);
  if (!parser.Parse()) {
    FAIL("Parser Error: " + parser.GetErrorMessage());
  }
  REQUIRE(!parser.HasError());

  auto instructions = parser.GetInstructions(); // Copy
  Resolver resolver(instructions, parser.GetLabels());

  CHECK(resolver.Resolve());

  // Check Instruction 0 (B target)
  // Target is at index 2 (Addr 8).
  // PC = 0 + 4 = 4.
  // Offset = 8 - 4 = 4.

  const auto &instr = instructions[0];
  CHECK(instr.Op == Opcode::B);
  CHECK(instr.Operands[0].Type == OperandType::Immediate);
  auto imm = std::get<ImmediateOperand>(instr.Operands[0].Value);
  CHECK(imm.Value == 4);
}

TEST_CASE("Resolver - Backward Branch") {
  // 0: loop:
  // 0: SUB R0, R0, #1
  // 1: BNE loop
  std::string source = "loop:\nSUB R0, R0, #1\nBNE loop";
  Lexer lexer(source);
  auto tokens = lexer.Tokenize();
  Parser parser(tokens);
  REQUIRE(parser.Parse());

  auto instructions = parser.GetInstructions();
  Resolver resolver(instructions, parser.GetLabels());

  CHECK(resolver.Resolve());

  // Instr 1 (BNE loop)
  // Target is 0 (Addr 0).
  // PC = 4 + 4 = 8.
  // Offset = 0 - 8 = -8.
  // -8 in uint64_t representation (2's complement cast)

  const auto &instr = instructions[1];

  // BNE offset.
  CHECK(instr.Operands.size() == 1);
  auto imm = std::get<ImmediateOperand>(instr.Operands[0].Value);

  // -8 cast to uint64_t
  CHECK(static_cast<std::int64_t>(imm.Value) == -8);
}

TEST_CASE("Resolver - Out of Range") {
  // Create many NOPs to push label out of range
  std::string source = "B target\n";
  for (int i = 0; i < 300; ++i)
    source += "NOP\n"; // 300 * 4 = 1200 bytes. > 1023.
  source += "target: HALT";

  Lexer lexer(source);
  auto tokens = lexer.Tokenize();
  Parser parser(tokens);
  REQUIRE(parser.Parse());

  auto instructions = parser.GetInstructions();
  Resolver resolver(instructions, parser.GetLabels());

  CHECK_FALSE(resolver.Resolve());
  CHECK(resolver.HasError());
  CHECK(resolver.GetErrorMessage().find("out of range") != std::string::npos);
}

TEST_CASE("Resolver - Undefined Label") {
  std::string source = "B nowhere";
  Lexer lexer(source);
  auto tokens = lexer.Tokenize();
  Parser parser(tokens);
  REQUIRE(parser.Parse());

  auto instructions = parser.GetInstructions();
  Resolver resolver(instructions, parser.GetLabels());

  CHECK_FALSE(resolver.Resolve());
  CHECK(resolver.HasError());
  CHECK(resolver.GetErrorMessage().find("Undefined Symbol") !=
        std::string::npos);
}
