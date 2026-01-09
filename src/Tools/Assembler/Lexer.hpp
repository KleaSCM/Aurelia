/**
 * Assembler Lexer Module.
 *
 * Responsible for tokenizing assembly source code into structured tokens.
 * Handles mnemonics, registers, immediates, labels, and directives.
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#pragma once

#include "Core/Types.hpp"
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace Aurelia::Tools::Assembler {

enum class TokenType {
  Mnemonic,     // ADD, SUB, MOV, etc.
  Register,     // R0-R15, SP, PC, LR
  Immediate,    // #123, #0xFF
  Label,        // loop:
  LabelRef,     // loop
  Directive,    // .data, .text
  Comma,        // ,
  Colon,        // : (Used internally for Label detection)
  LeftBracket,  // [
  RightBracket, // ]
  NewLine,      // \n
  String,       // "Hello"
  EndOfFile,    // EOF
  Unknown
};

struct Token {
  TokenType Type;
  std::string Text;
  std::optional<std::uint64_t> Value; // For Immediates
  std::size_t Line;
  std::size_t Column;

  bool operator==(const Token &other) const = default;
};

class Lexer {
public:
  explicit Lexer(std::string_view source);

  std::vector<Token> Tokenize();

private:
  char Peek(std::size_t offset = 0) const;
  char Advance();
  bool IsAtEnd() const;

  void SkipWhitespace();
  void SkipComment();

  Token ScanToken();
  Token ScanIdentifier();
  Token ScanNumber();
  Token ScanString();
  TokenType CheckKeyword(std::string_view text) const;

  std::string_view m_Source;
  std::size_t m_Current = 0;
  std::size_t m_Line = 1;
  std::size_t m_LineStart = 0;
};

} // namespace Aurelia::Tools::Assembler
