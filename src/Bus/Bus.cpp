/**
 * Bus Implementation.
 *
 * Manages the routing of Data, Address, and Control signals between the CPU and
 * Peripherals. Implements a cycle-accurate state machine for bus arbitration
 * and timing.
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#include <bit>

#include "Bus/Bus.hpp"
#include "Core/BitManip.hpp"

namespace Aurelia::Bus {

void Bus::ConnectDevice(IBusDevice *Device) { Devices.push_back(Device); }

void Bus::SetAddress(Core::Address Address) { State.AddrBus = Address; }

void Bus::SetData(Core::Data Data) { State.DataBus = Data; }

void Bus::SetControl(ControlSignal Signal, bool Active) {
  /**
   * CONTROL SIGNAL MAP
   *
   * Control signals are One-Hot encoded in the enum for type safety,
   * but we store them as individual bits in the Control word for efficiency.
   *
   * NOTE (KleaSCM) We use countr_zero to map the 1-hot value to a bit index
   * before setting/clearing the bit in the state control field.
   */
  auto bitIndex = static_cast<std::size_t>(
      std::countr_zero(static_cast<Core::Byte>(Signal)));

  if (Active) {
    State.Control = Core::SetBit(State.Control, bitIndex);
  } else {
    State.Control = Core::ClearBit(State.Control, bitIndex);
  }
}

const BusState &Bus::GetState() const { return State; }

bool Bus::IsBusy() const {
  auto waitBit = static_cast<std::size_t>(
      std::countr_zero(static_cast<Core::Byte>(ControlSignal::Wait)));
  return Core::CheckBit(State.Control, waitBit);
}

bool Bus::Read(Core::Address Address, Core::Data &OutData) {
  for (auto *device : Devices) {
    if (device->IsAddressInRange(Address)) {
      return device->OnRead(Address, OutData);
    }
  }
  return false;
}

bool Bus::Write(Core::Address Address, Core::Data InData) {
  for (auto *device : Devices) {
    if (device->IsAddressInRange(Address)) {
      return device->OnWrite(Address, InData);
    }
  }

  /**
   * BUS FAULT (DEBUG)
   *
   * No device mapped to this address.
   * In a real system this would trigger a bus fault exception.
   */
  return false;
}

void Bus::OnTick() {
  auto readBit = static_cast<std::size_t>(
      std::countr_zero(static_cast<Core::Byte>(ControlSignal::Read)));
  auto writeBit = static_cast<std::size_t>(
      std::countr_zero(static_cast<Core::Byte>(ControlSignal::Write)));

  bool isRead = Core::CheckBit(State.Control, readBit);
  bool isWrite = Core::CheckBit(State.Control, writeBit);

  /**
   * IDLE CHECK
   *
   * Optimization: If no control lines are active (Read/Write), the bus is idle.
   * We can skip address decoding to save simulation cycles.
   */
  if (!isRead && !isWrite) {
    return;
  }

  IBusDevice *target = nullptr;
  for (auto *dev : Devices) {
    if (dev->IsAddressInRange(State.AddrBus)) {
      target = dev;
      break;
    }
  }

  if (!target) {
    /**
     * ADDRESS DECODING FAILURE
     *
     * Critical Hardware Fault: Address on bus does not map to any device.
     * In real hardware, this might hang the bus or trigger a specialized error
     * interrupt. We assert the Error control line.
     */
    SetControl(ControlSignal::Error, true);
    return;
  }

  bool done = false;
  if (isRead) {
    done = target->OnRead(State.AddrBus, State.DataBus);
  } else if (isWrite) {
    done = target->OnWrite(State.AddrBus, State.DataBus);
  }

  /**
   * WAIT STATE MANAGEMENT
   *
   * Devices assert wait signals to hold the bus until they are ready.
   * We propagate this to the Wait control line.
   * - Done = true  -> Wait = false (Ready)
   * - Done = false -> Wait = true  (Busy)
   */
  SetControl(ControlSignal::Wait, !done);
}

} // namespace Aurelia::Bus
