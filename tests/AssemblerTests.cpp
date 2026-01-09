/**
 * Assembler Lexer Tests.
 *
 * Verifies that the Lexer correctly tokenizes assembly source code.
 * Ensures strict adherence to token types and line/column tracking.
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#include "Tools/Assembler/Lexer.hpp"
#include <catch2/catch_test_macros.hpp>

using namespace Aurelia::Tools::Assembler;

TEST_CASE("Lexer - Tokenize Simple Instruction") {
  Lexer lexer("ADD R1, R2, R3");
  auto tokens = lexer.Tokenize();

  REQUIRE(tokens.size() == 7);

  // ADD
  CHECK(tokens[0].Type == TokenType::Mnemonic);
  CHECK(tokens[0].Text == "ADD");

  // R1
  CHECK(tokens[1].Type == TokenType::Register);
  CHECK(tokens[1].Text == "R1");

  // ,
  CHECK(tokens[2].Type == TokenType::Comma);

  // R2
  CHECK(tokens[3].Type == TokenType::Register);
  CHECK(tokens[3].Text == "R2");

  // ,
  CHECK(tokens[4].Type == TokenType::Comma);

  // R3
  CHECK(tokens[5].Type == TokenType::Register);
  CHECK(tokens[5].Text == "R3");

  // EOF
  CHECK(tokens[6].Type == TokenType::EndOfFile);
}

TEST_CASE("Lexer - Tokenize Immediate Values") {
  Lexer lexer("MOV R0, #10\nMOV R1, #0xFF");
  auto tokens = lexer.Tokenize();

  // Check #10
  CHECK(tokens[3].Type == TokenType::Immediate);
  CHECK(tokens[3].Value.has_value());
  CHECK(tokens[3].Value.value() == 10);

  // Check #0xFF
  // Check #0xFF
  CHECK(tokens[8].Type == TokenType::Immediate);
  CHECK(tokens[8].Value.has_value());
  CHECK(tokens[8].Value.value() == 255);
}

TEST_CASE("Lexer - Labels and Directives") {
  Lexer lexer(".data\nloop:\n  B loop");
  auto tokens = lexer.Tokenize();

  // .data
  CHECK(tokens[0].Type == TokenType::Directive);
  CHECK(tokens[0].Text == ".data");

  // \n
  CHECK(tokens[1].Type == TokenType::NewLine);

  // loop:
  CHECK(tokens[2].Type == TokenType::Label);
  CHECK(tokens[2].Text == "loop"); // Colon stripped

  // \n
  CHECK(tokens[3].Type == TokenType::NewLine);

  // B
  CHECK(tokens[4].Type == TokenType::Mnemonic);
  CHECK(tokens[4].Text == "B");

  // loop (Reference)
  CHECK(tokens[5].Type == TokenType::LabelRef);
  CHECK(tokens[5].Text == "loop");
}

TEST_CASE("Lexer - Comments and Whitespace") {
  Lexer lexer("ADD R0, R1 ; This is a comment\nSUB R2, R3");
  auto tokens = lexer.Tokenize();

  CHECK(tokens[0].Text == "ADD");
  CHECK(tokens[3].Text == "R1");
  CHECK(tokens[4].Type == TokenType::NewLine);
  CHECK(tokens[5].Text == "SUB");
}

TEST_CASE("Lexer - Brackets (Memory Access)") {
  Lexer lexer("LDR R0, [R1]");
  auto tokens = lexer.Tokenize();

  // LDR, R0, ,, [, R1, ]
  CHECK(tokens[5].Type == TokenType::RightBracket);
}

TEST_CASE("Lexer - Features: Bin, Neg, Strings") {
  // Binary: #0b1010 -> 10
  // Negative: #-5 -> 0xFF...FB
  // String: "Hello"
  Lexer lexer("MOV R0, #0b1010\nADD R1, #-5\n.string \"Hello World\"");
  auto tokens = lexer.Tokenize();

  // Line 1: MOV, R0, ,, #0b1010, \n
  CHECK(tokens[3].Value.value() == 10);

  // Line 2: ADD, R1, ,, #-5, \n
  // index: 4(\n), 5(ADD), 6(R1), 7(,), 8(#-5)
  CHECK(tokens[8].Text == "#-5");
  CHECK(tokens[8].Value.value() == static_cast<std::uint64_t>(-5));

  // Line 3: .string, "Hello World", EOF
  // index: 9(\n), 10(.string), 11("Hello World")
  CHECK(tokens[10].Type == TokenType::Directive);
  CHECK(tokens[11].Type == TokenType::String);
  CHECK(tokens[11].Text == "Hello World");
}

TEST_CASE("Lexer - High Registers") {
  Lexer lexer("MOV R16, R31");
  auto tokens = lexer.Tokenize();

  CHECK(tokens[1].Type == TokenType::Register);
  CHECK(tokens[1].Text == "R16");

  CHECK(tokens[3].Type == TokenType::Register);
  CHECK(tokens[3].Text == "R31");
}
