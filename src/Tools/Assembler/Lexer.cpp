/**
 * Assembler Lexer Implementation.
 *
 * Implements the tokenization logic for the Aurelia Assembly language.
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#include "Tools/Assembler/Lexer.hpp"
#include <algorithm>
#include <cctype>
#include <charconv>
#include <iostream>
#include <unordered_map>

namespace Aurelia::Tools::Assembler {

Lexer::Lexer(std::string_view source) : m_Source(source) {}

std::vector<Token> Lexer::Tokenize() {
  std::vector<Token> tokens;
  while (!IsAtEnd()) {
    SkipWhitespace();
    if (IsAtEnd())
      break;

    Token token = ScanToken();
    if (token.Type != TokenType::Unknown) {
      tokens.push_back(token);
    }
  }
  // Explicit EOF token ensures parser knows when stream ends without checking
  // vector bounds.
  tokens.push_back({TokenType::EndOfFile, "", std::nullopt, m_Line,
                    m_Current - m_LineStart});
  return tokens;
}

char Lexer::Peek(std::size_t offset) const {
  if (m_Current + offset >= m_Source.length())
    return '\0';
  return m_Source[m_Current + offset];
}

char Lexer::Advance() {
  if (IsAtEnd())
    return '\0';
  return m_Source[m_Current++];
}

bool Lexer::IsAtEnd() const { return m_Current >= m_Source.length(); }

void Lexer::SkipWhitespace() {
  while (!IsAtEnd()) {
    char c = Peek();
    switch (c) {
    case ' ':
    case '\r':
    case '\t':
      Advance();
      break;
    case ';':
      SkipComment();
      break;
    default:
      return;
    }
  }
}

void Lexer::SkipComment() {
  while (!IsAtEnd() && Peek() != '\n') {
    Advance();
  }
}

Token Lexer::ScanToken() {
  std::size_t start = m_Current;
  char c = Advance();
  std::size_t column = start - m_LineStart + 1;

  switch (c) {
  case '"':
    return ScanString();
  case '\n':
    m_Line++;
    m_LineStart = m_Current; // Reset column counter base
    return {TokenType::NewLine, "\\n", std::nullopt, m_Line - 1, column};
  case ',':
    return {TokenType::Comma, ",", std::nullopt, m_Line, column};
  case '[':
    return {TokenType::LeftBracket, "[", std::nullopt, m_Line, column};
  case ']':
    return {TokenType::RightBracket, "]", std::nullopt, m_Line, column};
  case ':':
    return {TokenType::Colon, ":", std::nullopt, m_Line, column};
  case '.':
    // Directive start
    if (std::isalpha(Peek())) {
      while (std::isalnum(Peek()))
        Advance();
      return {TokenType::Directive,
              std::string(m_Source.substr(start, m_Current - start)),
              std::nullopt, m_Line, column};
    }
    return {TokenType::Unknown, std::string(1, c), std::nullopt, m_Line,
            column};
  case '#':
    return ScanNumber();
  default:
    if (std::isalpha(c)) {
      // Backtrack one char to handle full identifier scan
      m_Current--;
      return ScanIdentifier();
    }
    break;
  }

  return {TokenType::Unknown, std::string(1, c), std::nullopt, m_Line, column};
}

Token Lexer::ScanNumber() {
  std::size_t start = m_Current; // Skip the '#'
  std::size_t column =
      start - m_LineStart + 1; // Current points to first digit (Fix: 1-based)

  // Check for Sign: #-10
  bool isNegative = false;
  if (Peek() == '-') {
    isNegative = true;
    Advance();
  } else if (Peek() == '+') {
    Advance();
  }

  // Check for hex '0x' or binary '0b'
  bool isHex = false;
  bool isBin = false;
  if (!isNegative && Peek() == '0') {
    if (std::tolower(Peek(1)) == 'x') {
      isHex = true;
      Advance();
      Advance();
    } else if (std::tolower(Peek(1)) == 'b') {
      isBin = true;
      Advance();
      Advance();
    }
  }

  std::size_t numStart = m_Current;
  while (std::isxdigit(Peek())) {
    Advance();
  }

  const char *first = m_Source.data() + numStart;
  const char *last = m_Source.data() + m_Current;

  int base = 10;
  if (isHex)
    base = 16;
  else if (isBin)
    base = 2;

  std::int64_t value = 0;
  auto result = std::from_chars(first, last, value, base);

  std::string fullText(m_Source.substr(start, m_Current - start));

  if (result.ec != std::errc()) {
    // Overflow or invalid
    return {TokenType::Unknown, fullText, std::nullopt, m_Line, column};
  }

  if (isNegative) {
    value = -value;
  }

  return {TokenType::Immediate, "#" + fullText,
          static_cast<std::uint64_t>(value), m_Line, column};
}

Token Lexer::ScanString() {
  std::size_t start = m_Current - 1; // Include opening quote
  std::size_t column = start - m_LineStart + 1;

  while (Peek() != '"' && !IsAtEnd()) {
    if (Peek() == '\n')
      m_Line++;
    Advance();
  }

  if (IsAtEnd()) {
    return {TokenType::Unknown, "Unterminated String", std::nullopt, m_Line,
            column};
  }

  Advance(); // Consume closing quote

  // Store content *without* quotes
  std::string text =
      std::string(m_Source.substr(start + 1, m_Current - start - 2));

  return {TokenType::String, text, std::nullopt, m_Line, column};
}

Token Lexer::ScanIdentifier() {
  std::size_t start = m_Current;
  std::size_t column = start - m_LineStart + 1;

  while (std::isalnum(Peek()) || Peek() == '_') {
    Advance();
  }

  std::string text(m_Source.substr(start, m_Current - start));
  TokenType type = CheckKeyword(text);

  // Check for Label definition (Identifier followed by colon)
  if (Peek() == ':') {
    Advance(); // Consume ':'
    return {TokenType::Label, text, std::nullopt, m_Line,
            column}; // Return name without colon as label
  }

  return {type, text, std::nullopt, m_Line, column};
}

TokenType Lexer::CheckKeyword(std::string_view text) const {
  // Uppercase for case-insensitive check
  std::string upperText;
  upperText.reserve(text.length());
  for (char c : text)
    upperText += static_cast<char>(std::toupper(c));

  static const std::unordered_map<std::string, TokenType> keywords = {
      {"ADD", TokenType::Mnemonic},  {"SUB", TokenType::Mnemonic},
      {"AND", TokenType::Mnemonic},  {"OR", TokenType::Mnemonic},
      {"XOR", TokenType::Mnemonic},  {"LSL", TokenType::Mnemonic},
      {"LSR", TokenType::Mnemonic},  {"ASR", TokenType::Mnemonic},
      {"MOV", TokenType::Mnemonic},  {"LDR", TokenType::Mnemonic},
      {"STR", TokenType::Mnemonic},  {"B", TokenType::Mnemonic},
      {"BEQ", TokenType::Mnemonic},  {"BNE", TokenType::Mnemonic},
      {"CMP", TokenType::Mnemonic},  {"NOP", TokenType::Mnemonic},
      {"HALT", TokenType::Mnemonic},

      {"R0", TokenType::Register},   {"R1", TokenType::Register},
      {"R2", TokenType::Register},   {"R3", TokenType::Register},
      {"R4", TokenType::Register},   {"R5", TokenType::Register},
      {"R6", TokenType::Register},   {"R7", TokenType::Register},
      {"R8", TokenType::Register},   {"R9", TokenType::Register},
      {"R10", TokenType::Register},  {"R11", TokenType::Register},
      {"R12", TokenType::Register},  {"R13", TokenType::Register},
      {"R14", TokenType::Register},  {"R15", TokenType::Register},
      {"R16", TokenType::Register},  {"R17", TokenType::Register},
      {"R18", TokenType::Register},  {"R19", TokenType::Register},
      {"R20", TokenType::Register},  {"R21", TokenType::Register},
      {"R22", TokenType::Register},  {"R23", TokenType::Register},
      {"R24", TokenType::Register},  {"R25", TokenType::Register},
      {"R26", TokenType::Register},  {"R27", TokenType::Register},
      {"R28", TokenType::Register},  {"R29", TokenType::Register},
      {"R30", TokenType::Register},  {"R31", TokenType::Register},
      {"SP", TokenType::Register},   {"LR", TokenType::Register},
      {"PC", TokenType::Register}};

  auto it = keywords.find(upperText);
  if (it != keywords.end()) {
    return it->second;
  }

  return TokenType::LabelRef; // Anything else is a reference to a label
}

} // namespace Aurelia::Tools::Assembler
