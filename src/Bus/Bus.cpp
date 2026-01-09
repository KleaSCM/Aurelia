/**
 * Bus Implementation.
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#include "Bus/Bus.hpp"
#include "Core/BitManip.hpp"

namespace Aurelia::Bus {

void Bus::ConnectDevice(IBusDevice *device) { m_Devices.push_back(device); }

void Bus::SetAddress(Core::Address addr) { m_State.AddrBus = addr; }

void Bus::SetData(Core::Data data) { m_State.DataBus = data; }

void Bus::SetControl(ControlSignal signal, bool active) {
  if (active) {
    m_State.Control =
        Core::SetBit(m_State.Control, static_cast<std::size_t>(std::countr_zero(
                                          static_cast<Core::Byte>(signal))));
  } else {
    m_State.Control = Core::ClearBit(m_State.Control,
                                     static_cast<std::size_t>(std::countr_zero(
                                         static_cast<Core::Byte>(signal))));
  }
}

const BusState &Bus::GetState() const { return m_State; }

bool Bus::IsBusy() const {
  return Core::CheckBit(m_State.Control,
                        static_cast<std::size_t>(std::countr_zero(
                            static_cast<Core::Byte>(ControlSignal::Wait))));
}

void Bus::OnTick() {
  // 1. Identify active request
  bool isRead = Core::CheckBit(
      m_State.Control, static_cast<std::size_t>(std::countr_zero(
                           static_cast<Core::Byte>(ControlSignal::Read))));
  bool isWrite = Core::CheckBit(
      m_State.Control, static_cast<std::size_t>(std::countr_zero(
                           static_cast<Core::Byte>(ControlSignal::Write))));

  if (!isRead && !isWrite) {
    return; // Bus Idle
  }

  // 2. Find target device (Address Decoding)
  IBusDevice *target = nullptr;
  for (auto *dev : m_Devices) {
    if (dev->IsAddressInRange(m_State.AddrBus)) {
      target = dev;
      break;
    }
  }

  if (!target) {
    // Bus Error: No device found (TODO: trigger fault?)
    return;
  }

  // 3. Service Request
  bool done = false;
  if (isRead) {
    done = target->OnRead(m_State.AddrBus, m_State.DataBus);
  } else if (isWrite) {
    done = target->OnWrite(m_State.AddrBus, m_State.DataBus);
  }

  // 4. Update Wait Signal
  SetControl(ControlSignal::Wait, !done);
}

} // namespace Aurelia::Bus
