/**
 * Assembler Encoder Interface.
 *
 * The Encoder is responsible for the final stage of the assembly pipeline:
 * converting fully-resolved ParsedInstructions into raw binary machine code.
 *
 * ARCHITECTURE OVERVIEW:
 * This component implements the instruction encoding specification for the
 * Aurelia Instruction Set Architecture (ISA), which features a fixed 32-bit
 * instruction width with RISC-like encoding principles.
 *
 * ENCODING PIPELINE:
 * Input:  std::vector<ParsedInstruction> (from Resolver, labels resolved)
 * Output: std::vector<uint8_t> (ready-to-execute machine code)
 *
 * The encoding process validates operand types, maps operands to bit fields,
 * and emits instructions in little-endian byte order for direct memory loading.
 *
 * ERROR HANDLING:
 * The Encoder operates under the assumption that the Parser and Resolver have
 * already validated syntax and semantics. Encoding failures are treated as
 * internal errors, though the Encoder includes defensive checks.
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#pragma once

#include "Tools/Assembler/Parser.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace Aurelia::Tools::Assembler {

/**
 * @class Encoder
 * @brief Binary machine code generator for Aurelia ISA.
 *
 * The Encoder translates high-level ParsedInstructions into low-level
 * binary representation suitable for execution by the Aurelia CPU.
 *
 * DESIGN PRINCIPLES:
 * - Single-pass encoding: Each instruction encoded independently
 * - Stateless operation: No inter-instruction dependencies
 * - Defensive validation: Input assumed valid, but checked for safety
 *
 * THREAD SAFETY:
 * Not thread-safe. Instances should not be shared across threads.
 */
class Encoder {
public:
  /**
   * @brief Constructs an Encoder for a given instruction sequence.
   *
   * @param instructions Const reference to fully-resolved instructions.
   *                     This reference must remain valid for the lifetime
   *                     of the Encoder or until Encode() completes.
   *
   * NOTE (KleaSCM) The Encoder does NOT take ownership of instructions.
   * The caller must ensure the instruction vector outlives the Encoder.
   */
  explicit Encoder(const std::vector<ParsedInstruction> &instructions);

  /**
   * @brief Encodes all instructions into binary machine code.
   *
   * This method iterates through the instruction sequence, encoding each
   * instruction into a 32-bit word and appending it to the binary output
   * buffer in little-endian byte order.
   *
   * @return true if encoding succeeded, false on error.
   *
   * POSTCONDITIONS:
   * - On success: GetBinary() returns complete executable binary
   * - On failure: HasError() returns true, GetErrorMessage() provides details
   *
   * PERFORMANCE:
   * Time complexity: O(n) where n = instruction count
   * Space complexity: O(n) for output buffer (4 bytes per instruction)
   */
  [[nodiscard]] bool Encode();

  /**
   * @brief Retrieves the encoded binary output.
   *
   * @return Const reference to byte vector containing machine code.
   *
   * MEMORY LAYOUT:
   * Binary data is organized sequentially, with each 32-bit instruction
   * stored in little-endian format (LSB first).
   *
   * USAGE:
   * Call this method after successful Encode() to retrieve the binary
   * for writing to an output file or loading into memory.
   */
  [[nodiscard]] const std::vector<std::uint8_t> &GetBinary() const {
    return m_Binary;
  }

  /**
   * @brief Checks if an error occurred during encoding.
   * @return true if encoding failed, false otherwise.
   */
  [[nodiscard]] bool HasError() const { return m_HasError; }

  /**
   * @brief Retrieves the error message from the last encoding failure.
   * @return Human-readable error description with line number context.
   */
  [[nodiscard]] std::string GetErrorMessage() const { return m_ErrorMessage; }

private:
  /**
   * MEMBER VARIABLES
   */

  /// @brief Reference to source instruction sequence (immutable).
  const std::vector<ParsedInstruction> &m_Instructions;

  /// @brief Output binary buffer (grows during encoding).
  std::vector<std::uint8_t> m_Binary;

  /// @brief Error state flag (fail-fast on first error).
  bool m_HasError = false;

  /// @brief Detailed error message (includes line number and context).
  std::string m_ErrorMessage;

  /**
   * PRIVATE METHODS
   */

  /**
   * @brief Records an encoding error with context.
   *
   * @param instr Instruction that caused the error (for line number).
   * @param message Human-readable error description.
   *
   * NOTE (KleaSCM) Only the first error is recorded (fail-fast).
   * Subsequent errors are suppressed to avoid cascading error messages.
   */
  void Error(const ParsedInstruction &instr, const std::string &message);

  /**
   * @brief Encodes a single instruction into a 32-bit machine word.
   *
   * This method implements the core encoding logic, mapping ParsedInstruction
   * fields to the 32-bit instruction format defined by the Aurelia ISA.
   *
   * @param instr Fully-resolved instruction to encode.
   * @return 32-bit encoded instruction word.
   *
   * INSTRUCTION FORMAT:
   * ┌─────────┬─────┬─────┬─────┬────────────┐
   * │ Opcode  │ Rd  │ Rn  │ Rm  │ Immediate  │
   * │ [31:26] │[25:21]│[20:16]│[15:11]│ [10:0]    │
   * └─────────┴─────┴─────┴─────┴────────────┘
   *
   * ENCODING STRATEGY:
   * - Determine instruction format from operand count and types
   * - Extract register indices and immediate values from operands
   * - Map operands to appropriate bit fields
   * - Assemble fields into 32-bit word using bitwise operations
   *
   * ERROR HANDLING:
   * Sets m_HasError on failure. Returns 0 as safe fallback value.
   */
  [[nodiscard]] std::uint32_t EncodeInstruction(const ParsedInstruction &instr);
};

} // namespace Aurelia::Tools::Assembler
