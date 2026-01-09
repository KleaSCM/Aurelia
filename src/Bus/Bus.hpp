/**
 * System Bus.
 *
 * The central interconnect for Aurelia. Manages signal propagation and timing.
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#pragma once

#include "Bus/BusDefs.hpp"
#include "Bus/IBusDevice.hpp"
#include "Core/ITickable.hpp"
#include <vector>

namespace Aurelia::Bus {

class Bus : public Core::ITickable {
public:
  void ConnectDevice(IBusDevice *device);

  // Master Interface
  void SetAddress(Core::Address addr);
  void SetData(Core::Data data);
  void SetControl(ControlSignal signal, bool active);
  [[nodiscard]] const BusState &GetState() const;
  [[nodiscard]] bool IsBusy() const;

  // System Interface
  void OnTick() override;

private:
  std::vector<IBusDevice *> m_Devices;
  BusState m_State;

  // Internal latch to hold data during transfer
  Core::Data m_LatchedData = 0;
};

} // namespace Aurelia::Bus
