/**
 * Assembler Resolver.
 *
 * Performs semantic analysis and symbol resolution.
 * Pass 1: Assign addresses to instructions and define labels.
 * Pass 2: Resolve label references to immediate values (e.g., PC-relative
 * offsets).
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#pragma once

#include "Tools/Assembler/Parser.hpp"
#include "Tools/Assembler/SymbolTable.hpp"
#include <string>
#include <vector>

namespace Aurelia::Tools::Assembler {

class Resolver {
public:
  // The Resolver modifies instructions in-place to replace Labels with
  // Immediates.
  explicit Resolver(std::vector<ParsedInstruction> &instructions,
                    const std::vector<Parser::LabelDef> &labels);

  // Run Pass 1 and Pass 2
  [[nodiscard]] bool Resolve();

  [[nodiscard]] bool HasError() const { return m_HasError; }
  [[nodiscard]] std::string GetErrorMessage() const { return m_ErrorMessage; }

private:
  std::vector<ParsedInstruction> &m_Instructions;
  const std::vector<Parser::LabelDef> &m_Labels;
  SymbolTable m_SymbolTable;

  bool m_HasError = false;
  std::string m_ErrorMessage;

  // -- Logic --
  void BuildSymbolTable(); // Pass 1
  void ResolveOperands();  // Pass 2

  void Error(const ParsedInstruction &instr, const std::string &message);
};

} // namespace Aurelia::Tools::Assembler
