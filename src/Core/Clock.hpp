/**
 * System Clock.
 *
 * Maintains the global 64-bit cycle count.
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#pragma once

#include "Core/Types.hpp"

namespace Aurelia::Core {

class Clock {
public:
  void Tick();
  [[nodiscard]] TickCount GetTotalTicks() const;

private:
  TickCount m_CycleCount = 0;
};

} // namespace Aurelia::Core
