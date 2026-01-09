/**
 * System Implementation.
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#include "Core/System.hpp"

namespace Aurelia::Core {

void System::AddDevice(ITickable *device) { m_Devices.push_back(device); }

void System::Run(TickCount cycles) {
  for (TickCount i = 0; i < cycles; ++i) {
    // Update global time
    m_Clock.Tick();

    // Notify all devices
    for (auto *device : m_Devices) {
      device->OnTick();
    }
  }
}

const Clock &System::GetClock() const { return m_Clock; }

} // namespace Aurelia::Core
