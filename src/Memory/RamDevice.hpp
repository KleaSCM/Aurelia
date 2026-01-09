/**
 * RAM Device.
 *
 * Simulates a contiguous block of volatile memory with access latency.
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#pragma once

#include "Bus/IBusDevice.hpp"
#include <vector>

namespace Aurelia::Memory {

class RamDevice : public Bus::IBusDevice {
public:
  RamDevice(std::size_t sizeBytes, Core::TickCount latency = 1);

  [[nodiscard]] bool IsAddressInRange(Core::Address addr) const override;
  bool OnRead(Core::Address addr, Core::Data &outData) override;
  bool OnWrite(Core::Address addr, Core::Data inData) override;
  void OnTick() override;

  void SetBaseAddress(Core::Address baseAddr);

private:
  // Emulating physical storage
  std::vector<Core::Byte> m_Storage;
  Core::Address m_BaseAddr;
  std::size_t m_Size;

  // Latency Simulation
  Core::TickCount m_Latency;
  Core::TickCount m_CurrentWaitTicks;
  bool m_IsBusy = false;
};

} // namespace Aurelia::Memory
