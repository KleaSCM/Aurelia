/**
 * Assembler Parser Interface.
 *
 * Responsible for syntactic analysis of the assembly source code.
 * Converts a stream of tokens into a structured Abstract Syntax Tree (AST)
 * comprising Instructions, Directives, and Labels.
 *
 * Implements a recursive descent parser with strict syntax validation
 * and support for various addressing modes.
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#pragma once

#include "Cpu/InstructionDefs.hpp"
#include "Tools/Assembler/Lexer.hpp"
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_set>
#include <variant>
#include <vector>

namespace Aurelia::Tools::Assembler {

// -- AST Definitions --

enum class OperandType {
  Invalid,
  Register,
  Immediate,
  Memory, // [Rn, #Imm] etc.
  Label
};

struct RegisterOperand {
  std::uint8_t RegIndex; // 0-31
};

struct ImmediateOperand {
  std::uint64_t Value;
};

struct MemoryOperand {
  std::uint8_t BaseReg;
  std::int64_t Offset;
  bool PreIndexed;
  bool WriteBack;
};

struct LabelOperand {
  std::string Name;
};

using OperandValue = std::variant<RegisterOperand, ImmediateOperand,
                                  MemoryOperand, LabelOperand>;

struct Operand {
  OperandType Type;
  OperandValue Value;
};

struct ParsedInstruction {
  Cpu::Opcode Op;
  std::string Mnemonic; // Stored for error reporting context
  std::vector<Operand> Operands;
  std::size_t Line;
  std::size_t Column;
};

struct Symbol {
  std::string Name;
  std::size_t Address; // Byte offset in section
  bool IsGlobal;
};

class Parser {
public:
  explicit Parser(const std::vector<Token> &tokens);

  /**
   * @brief Parses the token stream into instructions and data.
   * @return true if parsing succeeded without errors.
   */
  bool Parse();

  /**
   * @brief Retrieves the list of parsed instructions.
   */
  [[nodiscard]] const std::vector<ParsedInstruction> &GetInstructions() const {
    return m_Instructions;
  }

  /**
   * @brief Retrieves the raw data segment bytes (.data).
   */
  [[nodiscard]] const std::vector<std::uint8_t> &GetDataSegment() const {
    return m_DataSegment;
  }

  /**
   * @brief Retrieves collected local labels.
   */
  struct LabelDef {
    std::string Name;
    std::size_t InstructionIndex;
  };
  [[nodiscard]] const std::vector<LabelDef> &GetLabels() const {
    return m_Labels;
  }

  [[nodiscard]] bool HasError() const { return m_HasError; }
  [[nodiscard]] std::string GetErrorMessage() const { return m_ErrorMessage; }

private:
  const std::vector<Token> &m_Tokens;
  std::size_t m_Current = 0;

  std::vector<ParsedInstruction> m_Instructions;
  std::vector<std::uint8_t> m_DataSegment;
  std::vector<LabelDef> m_Labels;
  std::unordered_set<std::string> m_DefinedLabels;

  bool m_HasError = false;
  std::string m_ErrorMessage;

  // -- Token Navigation --
  bool IsAtEnd() const;
  const Token &Peek() const;
  const Token &Previous() const;
  Token Advance();
  bool Check(TokenType type) const;
  bool Match(TokenType type);
  void Consume(TokenType type, const std::string &message);

  // -- Parsing Routines --
  void ParseStatement();
  void ParseLabel();
  void ParseDirective();
  void ParseInstruction();

  Operand ParseOperand();
  Operand ParseMemoryOperand(); // Handles [Rn, #Imm] grammar
  Operand ParseRegister();
  Operand ParseImmediate();
  Operand ParseLabelRef();

  void ParseStringDirective();

  // -- Error Handling --
  void Error(const Token &token, const std::string &message);
  void Synchronize();
};

} // namespace Aurelia::Tools::Assembler
