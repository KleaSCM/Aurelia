/**
 * Assembler Parser Verification Tests.
 *
 * Verifies the Parser correctly constructs the professional AST.
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#include "Tools/Assembler/Lexer.hpp"
#include "Tools/Assembler/Parser.hpp"
#include <catch2/catch_test_macros.hpp>
#include <variant>

using namespace Aurelia::Tools::Assembler;
using namespace Aurelia::Cpu;

TEST_CASE("Parser - Instruction Parsing") {
  // ADD R1, R2, R3
  std::string source = "ADD R1, R2, R3";
  Lexer lexer(source);
  auto tokens = lexer.Tokenize();

  Parser parser(tokens);
  bool result = parser.Parse();

  CHECK(result);
  const auto &instrs = parser.GetInstructions();
  REQUIRE(instrs.size() == 1);

  const auto &instr = instrs[0];
  CHECK(instr.Op == Opcode::ADD);
  REQUIRE(instr.Operands.size() == 3);

  // Operand 0: R1
  CHECK(instr.Operands[0].Type == OperandType::Register);
  auto reg0 = std::get<RegisterOperand>(instr.Operands[0].Value);
  CHECK(reg0.RegIndex == 1);

  // Operand 1: R2
  CHECK(instr.Operands[1].Type == OperandType::Register);
  auto reg1 = std::get<RegisterOperand>(instr.Operands[1].Value);
  CHECK(reg1.RegIndex == 2);
}

TEST_CASE("Parser - Immediate and Aliases") {
  // MOV SP, #0x1000
  std::string source = "MOV SP, #0x1000";
  Lexer lexer(source);
  auto tokens = lexer.Tokenize();

  Parser parser(tokens);
  CHECK(parser.Parse());

  const auto &instrs = parser.GetInstructions();
  REQUIRE(instrs.size() == 1);

  // SP -> R30
  auto reg = std::get<RegisterOperand>(instrs[0].Operands[0].Value);
  CHECK(reg.RegIndex == 30);

  // #0x1000
  CHECK(instrs[0].Operands[1].Type == OperandType::Immediate);
  auto imm = std::get<ImmediateOperand>(instrs[0].Operands[1].Value);
  CHECK(imm.Value == 0x1000);
}

TEST_CASE("Parser - Memory Operand") {
  // LDR R0, [R1, #4]
  std::string source = "LDR R0, [R1, #4]";
  Lexer lexer(source);
  auto tokens = lexer.Tokenize();

  Parser parser(tokens);
  CHECK(parser.Parse());

  const auto &instr = parser.GetInstructions()[0];
  CHECK(instr.Op == Opcode::LDR);

  // Operand 1 should be Memory [R1, #4]
  // It's index 1 (R0 is index 0)
  CHECK(instr.Operands[1].Type == OperandType::Memory);
  auto mem = std::get<MemoryOperand>(instr.Operands[1].Value);
  CHECK(mem.BaseReg == 1);
  CHECK(mem.Offset == 4);
}

TEST_CASE("Parser - String Directive") {
  std::string source = ".string \"Hi\"";
  Lexer lexer(source);
  auto tokens = lexer.Tokenize();

  Parser parser(tokens);
  CHECK(parser.Parse());

  const auto &data = parser.GetDataSegment();
  REQUIRE(data.size() == 3); // H, i, \0
  CHECK(data[0] == 'H');
  CHECK(data[1] == 'i');
  CHECK(data[2] == 0);
}

TEST_CASE("Parser - Error Reporting") {
  std::string source = "MOV R0"; // Missing operand
  // Wait, generic parsing doesn't check counts yet (Decoder/Encoder will)
  // But "MOV R0" followed by nothing is valid syntax-wise (Opcode + List)
  // Let's test Syntax Error: "MOV R0,, R1" or "[R0, R1]" (Reg offset not
  // supported yet) "LDR R0, [R1" (Missing bracket)

  std::string invalidSource = "LDR R0, [R1";
  Lexer lexer(invalidSource);
  auto tokens = lexer.Tokenize();

  Parser parser(tokens);
  CHECK_FALSE(parser.Parse());
  CHECK(parser.HasError());
  CHECK(parser.GetErrorMessage().find("Expected ']'") != std::string::npos);
}
