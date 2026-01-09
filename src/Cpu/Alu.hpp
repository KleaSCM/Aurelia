/**
 * Arithmetic Logic Unit.
 *
 * Stateless component that performs mathematical operations.
 * Calculates Result and Updates architectural flags (Z, N, C, V).
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#pragma once

#include "Cpu/AluDefs.hpp"

namespace Aurelia::Cpu {

class Alu {
public:
  // NOTE (KleaSCM) Pure function execution. Stateless.
  [[nodiscard]] static AluResult Execute(AluOp op, Core::Word a, Core::Word b,
                                         const Flags &currentFlags);
};

} // namespace Aurelia::Cpu
