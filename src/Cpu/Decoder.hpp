/**
 * Instruction Decoder.
 *
 * Decodes raw 32-bit instruction words into structured Instruction objects.
 * Handles bit extraction and field mapping.
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#pragma once

#include "Cpu/InstructionDefs.hpp"

namespace Aurelia::Cpu {

class Decoder {
public:
  [[nodiscard]] static Instruction Decode(std::uint32_t rawInstr);
};

} // namespace Aurelia::Cpu
