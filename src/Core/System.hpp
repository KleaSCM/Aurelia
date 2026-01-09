/**
 * System Manager.
 *
 * Orchestrates the simulation lifecycle and component updates.
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#pragma once

#include "Core/Clock.hpp"
#include "Core/ITickable.hpp"
#include <vector>

namespace Aurelia::Core {

class System {
public:
  void AddDevice(ITickable *device);
  void Run(TickCount cycles);
  [[nodiscard]] const Clock &GetClock() const;

private:
  Clock m_Clock;
  std::vector<ITickable *> m_Devices;
};

} // namespace Aurelia::Core
