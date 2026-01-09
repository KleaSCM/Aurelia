/**
 * Assembler Parser Implementation.
 *
 * Implements the recursive descent parsing logic for the assembly language.
 * Enforces grammar rules and constructs the AST.
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#include "Tools/Assembler/Parser.hpp"
#include <algorithm>
#include <charconv>
#include <unordered_map>

namespace Aurelia::Tools::Assembler {

Parser::Parser(const std::vector<Token> &tokens) : m_Tokens(tokens) {}

bool Parser::Parse() {
  while (!IsAtEnd()) {
    if (m_HasError)
      return false;
    ParseStatement();
  }
  return !m_HasError;
}

void Parser::ParseStatement() {
  // Skip empty lines
  if (Match(TokenType::NewLine))
    return;

  if (Check(TokenType::Label)) {
    ParseLabel();
    return;
  }

  if (Check(TokenType::Directive)) {
    ParseDirective();
    return;
  }

  if (Check(TokenType::Mnemonic)) {
    ParseInstruction();
    return;
  }

  // Unexpected token
  Error(Peek(), "Unexpected token in statement: " + Peek().Text);
  Synchronize();
}

void Parser::ParseLabel() {
  if (Check(TokenType::Label)) {
    Token token = Advance();
    // Check duplicates
    if (m_DefinedLabels.contains(token.Text)) {
      Error(token, "Duplicate label definition: " + token.Text);
      return;
    }

    m_Labels.push_back({token.Text, m_Instructions.size()});
    m_DefinedLabels.insert(token.Text);

    // Lexer strips colon.
  }
}

void Parser::ParseDirective() {
  Token token = Advance();
  std::string dir = token.Text;

  // Normalize directive name
  std::transform(dir.begin(), dir.end(), dir.begin(),
                 [](unsigned char c) { return std::tolower(c); });

  if (dir == ".string") {
    ParseStringDirective();
  } else if (dir == ".data" || dir == ".text") {
    // Section switching placeholder
  } else {
    Error(token, "Unknown directive: " + token.Text);
  }

  // Directives end logic usually
  if (!IsAtEnd())
    Consume(TokenType::NewLine, "Expected newline after directive");
}

void Parser::ParseStringDirective() {
  if (!Match(TokenType::String)) {
    Error(Peek(), "Expected string literal");
    return;
  }

  std::string raw = Previous().Text;
  if (raw.size() >= 2 && raw.front() == '"' && raw.back() == '"') {
    raw = raw.substr(1, raw.size() - 2);
  }

  for (size_t i = 0; i < raw.size(); ++i) {
    char c = raw[i];
    if (c == '\\' && i + 1 < raw.size()) {
      char next = raw[i + 1];
      switch (next) {
      case 'n':
        m_DataSegment.push_back('\n');
        break;
      case 't':
        m_DataSegment.push_back('\t');
        break;
      case 'r':
        m_DataSegment.push_back('\r');
        break;
      case '0':
        m_DataSegment.push_back('\0');
        break;
      case '\\':
        m_DataSegment.push_back('\\');
        break;
      case '"':
        m_DataSegment.push_back('"');
        break;
      default:
        m_DataSegment.push_back(static_cast<std::uint8_t>(c)); // Keep backslash
        m_DataSegment.push_back(static_cast<std::uint8_t>(next));
        break;
      }
      if (next == 'n' || next == 't' || next == 'r' || next == '0' ||
          next == '\\' || next == '"') {
        i++; // Skip next char
      }
    } else {
      m_DataSegment.push_back(static_cast<std::uint8_t>(c));
    }
  }
  m_DataSegment.push_back(0); // Null terminator
}

void Parser::ParseInstruction() {
  Token mnemonicToken = Advance();
  ParsedInstruction instr;
  instr.Mnemonic = mnemonicToken.Text;
  instr.Line = mnemonicToken.Line;
  instr.Column = mnemonicToken.Column;

  // Resolve Opcode
  static const std::unordered_map<std::string, Cpu::Opcode> OpMap = {
      {"ADD", Cpu::Opcode::ADD},  {"SUB", Cpu::Opcode::SUB},
      {"AND", Cpu::Opcode::AND},  {"OR", Cpu::Opcode::OR},
      {"XOR", Cpu::Opcode::XOR},  {"LSL", Cpu::Opcode::LSL},
      {"LSR", Cpu::Opcode::LSR},  {"ASR", Cpu::Opcode::ASR},
      {"MOV", Cpu::Opcode::MOV},  {"LDR", Cpu::Opcode::LDR},
      {"STR", Cpu::Opcode::STR},  {"B", Cpu::Opcode::B},
      {"BEQ", Cpu::Opcode::BEQ},  {"BNE", Cpu::Opcode::BNE},
      {"CMP", Cpu::Opcode::CMP},  {"NOP", Cpu::Opcode::NOP},
      {"HALT", Cpu::Opcode::Halt}};

  std::string upper = instr.Mnemonic;
  std::transform(upper.begin(), upper.end(), upper.begin(),
                 [](unsigned char c) { return std::toupper(c); });

  auto it = OpMap.find(upper);
  if (it != OpMap.end()) {
    instr.Op = it->second;
  } else {
    Error(mnemonicToken, "Unknown Mnemonic");
    return;
  }

  // Parse Operands (comma separated)
  if (!Check(TokenType::NewLine) && !IsAtEnd()) {
    do {
      auto op = ParseOperand();
      if (op.Type == OperandType::Invalid) {
        // Error already reported
        Synchronize();
        return;
      }
      instr.Operands.push_back(op);
    } while (Match(TokenType::Comma));
  }

  if (!IsAtEnd())
    Consume(TokenType::NewLine, "Expected newline after instruction");

  if (!m_HasError) {
    m_Instructions.push_back(instr);
  }
}

Operand Parser::ParseOperand() {
  if (Check(TokenType::LeftBracket)) {
    return ParseMemoryOperand();
  }
  if (Check(TokenType::Register)) {
    return ParseRegister();
  }
  if (Check(TokenType::Immediate)) {
    return ParseImmediate();
  }
  if (Check(TokenType::LabelRef)) {
    return ParseLabelRef();
  }

  Error(Peek(), "Expected Operand (Register, Immediate, Memory, or Label)");
  return {OperandType::Invalid, {}};
}

Operand Parser::ParseMemoryOperand() {
  Consume(TokenType::LeftBracket, "Expected '['");

  // Base Register (Required)
  Operand baseOp = ParseRegister();
  if (baseOp.Type == OperandType::Invalid)
    return {OperandType::Invalid, {}};

  // Safety check - though ParseRegister ensures Type=Register or Invalid
  if (!std::holds_alternative<RegisterOperand>(baseOp.Value)) {
    Error(Previous(), "Memory operand must start with a Register");
    return {OperandType::Invalid, {}};
  }
  RegisterOperand baseReg = std::get<RegisterOperand>(baseOp.Value);

  // Optional Offset: , #Imm
  std::int64_t offset = 0;
  if (Match(TokenType::Comma)) {
    Operand offsetOp = ParseImmediate();
    if (offsetOp.Type == OperandType::Invalid)
      return {OperandType::Invalid, {}};

    if (std::holds_alternative<ImmediateOperand>(offsetOp.Value)) {
      offset = static_cast<std::int64_t>(
          std::get<ImmediateOperand>(offsetOp.Value).Value);
    } else {
      Error(Previous(), "Memory offset must be an Immediate");
      return {OperandType::Invalid, {}};
    }
  }

  Consume(TokenType::RightBracket, "Expected ']'");

  Operand op;
  op.Type = OperandType::Memory;
  op.Value = MemoryOperand{baseReg.RegIndex, offset, false, false};
  return op;
}

Operand Parser::ParseRegister() {
  if (!Check(TokenType::Register)) {
    Error(Peek(), "Expected Register");
    return {OperandType::Invalid, {}};
  }
  Token token = Advance(); // Consume Register token
  std::string text = token.Text;

  // Normalize
  std::string upper = text;
  std::transform(upper.begin(), upper.end(), upper.begin(),
                 [](unsigned char c) { return std::toupper(c); });

  int regIndex = 0;

  // Alias Handling
  if (upper == "SP")
    regIndex = 30;
  else if (upper == "LR")
    regIndex = 31;
  else if (upper == "PC")
    regIndex = 32; // Placeholder, not strictly gpr
  else if (upper.size() > 1 && upper[0] == 'R') {
    // R<num>
    auto result = std::from_chars(upper.data() + 1, upper.data() + upper.size(),
                                  regIndex);
    if (result.ec != std::errc()) {
      Error(token, "Invalid Register Format");
      return {OperandType::Invalid, {}};
    }
  } else {
    Error(token, "Unknown Register Name");
    return {OperandType::Invalid, {}};
  }

  Operand op;
  op.Type = OperandType::Register;
  op.Value = RegisterOperand{static_cast<std::uint8_t>(regIndex)};
  return op;
}

Operand Parser::ParseImmediate() {
  if (!Check(TokenType::Immediate)) {
    Error(Peek(), "Expected Immediate");
    return {OperandType::Invalid, {}};
  }
  Token token = Advance();
  // Lexer 'Value' holds the parsed number
  if (!token.Value.has_value()) {
    Error(token, "Immediate token missing numeric value");
    return {OperandType::Invalid, {}};
  }

  Operand op;
  op.Type = OperandType::Immediate;
  op.Value = ImmediateOperand{token.Value.value()};
  return op;
}

Operand Parser::ParseLabelRef() {
  if (!Check(TokenType::LabelRef)) {
    Error(Peek(), "Expected Label Reference");
    return {OperandType::Invalid, {}};
  }
  Token token = Advance();
  Operand op;
  op.Type = OperandType::Label;
  op.Value = LabelOperand{token.Text};
  return op;
}

// -- Navigation --

bool Parser::Match(TokenType type) {
  if (Check(type)) {
    Advance();
    return true;
  }
  return false;
}

bool Parser::Check(TokenType type) const {
  if (IsAtEnd())
    return false;
  return Peek().Type == type;
}

Token Parser::Advance() {
  if (!IsAtEnd())
    m_Current++;
  return Previous();
}

bool Parser::IsAtEnd() const { return Peek().Type == TokenType::EndOfFile; }

const Token &Parser::Peek() const {
  if (m_Current >= m_Tokens.size())
    return m_Tokens.back();
  return m_Tokens[m_Current];
}

const Token &Parser::Previous() const {
  if (m_Current == 0)
    return m_Tokens[0];
  return m_Tokens[m_Current - 1];
}

void Parser::Consume(TokenType type, const std::string &message) {
  if (Check(type)) {
    Advance();
    return;
  }
  Error(Peek(), message);
}

void Parser::Error(const Token &token, const std::string &message) {
  if (m_HasError)
    return; // Suppress cascade
  m_HasError = true;
  m_ErrorMessage = "[Line " + std::to_string(token.Line) + "] " + message;
}

void Parser::Synchronize() {
  Advance();
  while (!IsAtEnd()) {
    if (Previous().Type == TokenType::NewLine)
      return;
    Advance();
  }
}

} // namespace Aurelia::Tools::Assembler
