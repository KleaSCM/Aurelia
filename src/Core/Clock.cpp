/**
 * Clock Implementation.
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#include "Core/Clock.hpp"

namespace Aurelia::Core {

void Clock::Tick() { m_CycleCount++; }

TickCount Clock::GetTotalTicks() const { return m_CycleCount; }

} // namespace Aurelia::Core
