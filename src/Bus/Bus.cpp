/**
 * Bus Implementation.
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#include "Bus/Bus.hpp"
#include "Core/BitManip.hpp"
#include <iostream>

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

bool Bus::Read(Core::Address addr, Core::Data &outData) {
  for (auto *device : m_Devices) {
    if (device->IsAddressInRange(addr)) {
      return device->OnRead(addr, outData);
    }
  }
  return false;
}

bool Bus::Write(Core::Address addr, Core::Data inData) {
  for (auto *device : m_Devices) {
    if (device->IsAddressInRange(addr)) {
      return device->OnWrite(addr, inData);
    }
  }
  // DEBUG: No device found for this address
  std::cerr << "Bus::Write - No device for addr 0x" << std::hex << addr
            << " (devices: " << std::dec << m_Devices.size() << ")\n";
  return false;
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

  // 3. Service Request
  if (!target) {
    // NOTE (KleaSCM) Bus Fault. Address does not map to any device.
    SetControl(ControlSignal::Error, true);
    return;
  }

  // 4. Execute Transaction
  bool done = false;
  if (isRead) {
    done = target->OnRead(m_State.AddrBus, m_State.DataBus);
  } else if (isWrite) {
    done = target->OnWrite(m_State.AddrBus, m_State.DataBus);
  }

  // 5. Update Wait Signal
  SetControl(ControlSignal::Wait, !done);
}

} // namespace Aurelia::Bus
