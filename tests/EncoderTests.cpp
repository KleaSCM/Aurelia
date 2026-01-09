/**
 * Assembler Encoder Tests.
 *
 * Verifies binary machine code generation and error validation.
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com  */

#include "Tools/Assembler/Encoder.hpp"
#include "Tools/Assembler/Lexer.hpp"
#include "Tools/Assembler/Parser.hpp"
#include "Tools/Assembler/Resolver.hpp"
#include <catch2/catch_test_macros.hpp>

using namespace Aurelia::Tools::Assembler;
using namespace Aurelia::Cpu;

TEST_CASE("Encoder - Data Processing") {
  std::string source = "ADD R1, R2, R3";
  Lexer lexer(source);
  auto tokens = lexer.Tokenize();
  Parser parser(tokens);
  REQUIRE(parser.Parse());

  Encoder encoder(parser.GetInstructions());
  REQUIRE(encoder.Encode());

  auto bin = encoder.GetBinary();
  REQUIRE(bin.size() == 4);

  std::uint32_t val = 0;
  val |= bin[0];
  val |= (bin[1] << 8);
  val |= (bin[2] << 16);
  val |= (bin[3] << 24);

  CHECK(val == 0x04221800);
}

TEST_CASE("Encoder - Immediate Operation") {
  std::string source = "MOV R5, #255";
  Lexer lexer(source);
  auto tokens = lexer.Tokenize();
  Parser parser(tokens);
  REQUIRE(parser.Parse());

  Encoder encoder(parser.GetInstructions());
  REQUIRE(encoder.Encode());

  auto bin = encoder.GetBinary();
  std::uint32_t val = bin[0] | (bin[1] << 8) | (bin[2] << 16) | (bin[3] << 24);

  CHECK(val == 0x80A000FF);
}

TEST_CASE("Encoder - Memory Operation") {
  std::string source = "LDR R10, [R1, #16]";
  Lexer lexer(source);
  auto tokens = lexer.Tokenize();
  Parser parser(tokens);
  REQUIRE(parser.Parse());

  Encoder encoder(parser.GetInstructions());
  REQUIRE(encoder.Encode());

  auto bin = encoder.GetBinary();
  std::uint32_t val = bin[0] | (bin[1] << 8) | (bin[2] << 16) | (bin[3] << 24);

  CHECK(val == 0x41410010);
}

TEST_CASE("Encoder - Branch Resolved") {
  std::string source = "B target\ntarget:\nNOP";
  Lexer lexer(source);
  auto tokens = lexer.Tokenize();
  Parser parser(tokens);
  REQUIRE(parser.Parse());

  auto instructions = parser.GetInstructions();
  Resolver resolver(instructions, parser.GetLabels());
  REQUIRE(resolver.Resolve());

  Encoder encoder(instructions);
  REQUIRE(encoder.Encode());

  auto bin = encoder.GetBinary();
  REQUIRE(bin.size() == 8); // 2 instructions

  // First instruction: B with offset 0 (target is next instruction)
  // Branch offset = (4 - (0+4)) = 0
  std::uint32_t branch =
      bin[0] | (bin[1] << 8) | (bin[2] << 16) | (bin[3] << 24);
  std::uint32_t expectedBranch = (0x30 << 26); // B opcode, offset=0
  CHECK(branch == expectedBranch);
}

TEST_CASE("Encoder - Validation: Wrong Operand Count") {
  // Manually construct invalid instruction (bypassing parser)
  ParsedInstruction badInstr;
  badInstr.Op = Opcode::ADD;
  badInstr.Mnemonic = "ADD";
  badInstr.Line = 1;
  badInstr.Column = 1;

  // ADD requires 3 operands, provide only 2
  Operand op1{OperandType::Register, RegisterOperand{1}};
  Operand op2{OperandType::Register, RegisterOperand{2}};
  badInstr.Operands = {op1, op2};

  std::vector<ParsedInstruction> instrs = {badInstr};
  Encoder encoder(instrs);

  REQUIRE_FALSE(encoder.Encode());
  REQUIRE(encoder.HasError());
  CHECK(encoder.GetErrorMessage().find("requires exactly 3 operands") !=
        std::string::npos);
}

TEST_CASE("Encoder - Validation: Wrong Operand Type") {
  // MOV with immediate as destination (invalid)
  ParsedInstruction badInstr;
  badInstr.Op = Opcode::MOV;
  badInstr.Mnemonic = "MOV";
  badInstr.Line = 1;
  badInstr.Column = 1;

  Operand op1{OperandType::Immediate,
              ImmediateOperand{5}}; // Bad: dest must be register
  Operand op2{OperandType::Register, RegisterOperand{1}};
  badInstr.Operands = {op1, op2};

  std::vector<ParsedInstruction> instrs = {badInstr};
  Encoder encoder(instrs);

  REQUIRE_FALSE(encoder.Encode());
  REQUIRE(encoder.HasError());
  CHECK(encoder.GetErrorMessage().find("destination must be a register") !=
        std::string::npos);
}

TEST_CASE("Encoder - Validation: Immediate Out of Range") {
  // MOV with immediate > 2047 (11-bit range)
  ParsedInstruction badInstr;
  badInstr.Op = Opcode::MOV;
  badInstr.Mnemonic = "MOV";
  badInstr.Line = 1;
  badInstr.Column = 1;

  Operand op1{OperandType::Register, RegisterOperand{1}};
  Operand op2{OperandType::Immediate, ImmediateOperand{5000}}; // > 2047
  badInstr.Operands = {op1, op2};

  std::vector<ParsedInstruction> instrs = {badInstr};
  Encoder encoder(instrs);

  REQUIRE_FALSE(encoder.Encode());
  REQUIRE(encoder.HasError());
  CHECK(encoder.GetErrorMessage().find("out of range") != std::string::npos);
  CHECK(encoder.GetErrorMessage().find("5000") != std::string::npos);
}

TEST_CASE("Encoder - Validation: Memory Operation Wrong Type") {
  // LDR with register instead of memory operand
  ParsedInstruction badInstr;
  badInstr.Op = Opcode::LDR;
  badInstr.Mnemonic = "LDR";
  badInstr.Line = 1;
  badInstr.Column = 1;

  Operand op1{OperandType::Register, RegisterOperand{1}};
  Operand op2{OperandType::Register, RegisterOperand{2}}; // Should be Memory
  badInstr.Operands = {op1, op2};

  std::vector<ParsedInstruction> instrs = {badInstr};
  Encoder encoder(instrs);

  REQUIRE_FALSE(encoder.Encode());
  REQUIRE(encoder.HasError());
  CHECK(encoder.GetErrorMessage().find("memory syntax") != std::string::npos);
}

TEST_CASE("Encoder - Validation: Branch with Register") {
  // B with register instead of immediate
  ParsedInstruction badInstr;
  badInstr.Op = Opcode::B;
  badInstr.Mnemonic = "B";
  badInstr.Line = 1;
  badInstr.Column = 1;

  Operand op1{OperandType::Register, RegisterOperand{1}}; // Should be Immediate
  badInstr.Operands = {op1};

  std::vector<ParsedInstruction> instrs = {badInstr};
  Encoder encoder(instrs);

  REQUIRE_FALSE(encoder.Encode());
  REQUIRE(encoder.HasError());
  CHECK(encoder.GetErrorMessage().find("immediate offset") !=
        std::string::npos);
}

TEST_CASE("Encoder - Validation: NOP with Operands") {
  // NOP should have no operands
  ParsedInstruction badInstr;
  badInstr.Op = Opcode::NOP;
  badInstr.Mnemonic = "NOP";
  badInstr.Line = 1;
  badInstr.Column = 1;

  Operand op1{OperandType::Register, RegisterOperand{1}};
  badInstr.Operands = {op1};

  std::vector<ParsedInstruction> instrs = {badInstr};
  Encoder encoder(instrs);

  REQUIRE_FALSE(encoder.Encode());
  REQUIRE(encoder.HasError());
  CHECK(encoder.GetErrorMessage().find("takes no operands") !=
        std::string::npos);
}

TEST_CASE("Encoder - Validation: CMP Uses Rn Not Rd") {
  // Verify CMP correctly maps first operand to Rn field
  std::string source = "CMP R3, #10";
  Lexer lexer(source);
  auto tokens = lexer.Tokenize();
  Parser parser(tokens);
  REQUIRE(parser.Parse());

  Encoder encoder(parser.GetInstructions());
  REQUIRE(encoder.Encode());

  auto bin = encoder.GetBinary();
  std::uint32_t val = bin[0] | (bin[1] << 8) | (bin[2] << 16) | (bin[3] << 24);

  // CMP: Op=0x09, Rd=0, Rn=3, Rm=0, Imm=10
  // [001001][00000][00011][00000][00000001010]
  std::uint32_t expected = (0x09 << 26) | (3 << 16) | 10;
  CHECK(val == expected);
}
