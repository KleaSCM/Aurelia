/**
 * Assembler Encoder Tests.
 *
 * Verifies binary machine code generation for all instruction formats.
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#include "Tools/Assembler/Encoder.hpp"
#include "Tools/Assembler/Lexer.hpp"
#include "Tools/Assembler/Parser.hpp"
#include <catch2/catch_test_macros.hpp>

using namespace Aurelia::Tools::Assembler;
using namespace Aurelia::Cpu;

TEST_CASE("Encoder - Data Processing") {
  // ADD R1, R2, R3
  // Op=0x01, Rd=1, Rn=2, Rm=3
  // [000001] [00001] [00010] [00011] [000...0]
  // 0x01 << 26 = 0x04000000
  // 1 << 21    = 0x00200000
  // 2 << 16    = 0x00020000
  // 3 << 11    = 0x00001800
  // Total: 0x04221800

  std::string source = "ADD R1, R2, R3";
  Lexer lexer(source);
  auto tokens = lexer.Tokenize(); // Store tokens to avoid dangling reference
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
  // MOV R5, #255
  // Op=0x20, Rd=5, Imm=255 (0xFF)
  // [100000] [00101] [00000] [00000] [00011111111]
  // 0x20 << 26 = 0x80000000
  // 5 << 21    = 0x00A00000
  // Imm        = 0x000000FF
  // Total: 0x80A000FF

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
  // LDR R10, [R1, #16]
  // Op=0x10, Rd=10, Rn=1, Imm=16
  // [010000] [01010] [00001] [00000] [00000010000]
  // 0x10 << 26 = 0x40000000
  // 10 << 21   = 0x01400000
  // 1  << 16   = 0x00010000
  // 16         = 0x00000010
  // Total: 0x41410010

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
