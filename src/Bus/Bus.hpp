/**
 * System Bus.
 *
 * The central interconnect for Aurelia. Manages signal propagation and timing.
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#pragma once

#include <vector>

#include "Bus/BusDefs.hpp"
#include "Bus/IBusDevice.hpp"
#include "Core/ITickable.hpp"

namespace Aurelia::Bus {

class Bus : public Core::ITickable {
public:
  void ConnectDevice(IBusDevice *Device);

  // Master Interface
  void SetAddress(Core::Address Address);
  void SetData(Core::Data Data);
  void SetControl(ControlSignal Signal, bool Active);

  [[nodiscard]] const BusState &GetState() const;
  [[nodiscard]] bool IsBusy() const;

  // Debug / DMA Access (Bypasses timing)

  // NOTE (KleaSCM) These methods bypass the cycle-accurate simulation
  // and are intended for debugging or instant transfers (DMA).
  bool Read(Core::Address Address, Core::Data &OutData);
  bool Write(Core::Address Address, Core::Data InData);

  // System Interface
  void OnTick() override;

private:
  std::vector<IBusDevice *> Devices;
  BusState State;

  // Internal latch to hold data during transfer
  Core::Data LatchedData = 0;
};

} // namespace Aurelia::Bus
