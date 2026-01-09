/**
 * Aurelia Mouse Controller Implementation.
 *
 * Implements the relative motion accumulation logic.
 *
 * ALGORITHM:
 * - Accumulators (m_AccX, m_AccY) sum up all incoming events.
 * - Reading a Data register returns the current sum and atomically resets it to
 * 0.
 * - This ensures no movement is lost between polls.
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#include "Peripherals/MouseDevice.hpp"
#include <limits>

namespace Aurelia::Peripherals {

MouseDevice::MouseDevice() {}

void MouseDevice::ConnectPic(PicDevice *pic) { m_Pic = pic; }

bool MouseDevice::IsAddressInRange(Core::Address addr) const {
  return addr >= BaseAddr && addr < (BaseAddr + 0x1000);
}

bool MouseDevice::OnRead(Core::Address addr, Core::Data &outData) {
  Core::Address offset = addr - BaseAddr;

  switch (offset) {
  case StatusOffset: {
    // Packet Ready if any movement pending
    bool ready = (m_AccX != 0) || (m_AccY != 0);
    std::uint32_t status = ready ? StatusPacketReady : 0;
    if (m_OverflowX)
      status |= StatusXOverflow;
    if (m_OverflowY)
      status |= StatusYOverflow;
    outData = status;
    return true;
  }
  case DataXOffset: {
    // Return Accumulator and Reset (Clear-on-Read)
    outData = static_cast<Core::Data>(m_AccX);
    m_AccX = 0;
    m_OverflowX = false; // Clear overflow status
    return true;
  }
  case DataYOffset: {
    outData = static_cast<Core::Data>(m_AccY);
    m_AccY = 0;
    m_OverflowY = false; // Clear overflow status
    return true;
  }
  case ButtonsOffset: {
    outData = m_Buttons;
    return true;
  }
  case ControlOffset: {
    outData = m_Control;
    return true;
  }
  default:
    return false;
  }
}

bool MouseDevice::OnWrite(Core::Address addr, Core::Data inData) {
  Core::Address offset = addr - BaseAddr;

  switch (offset) {
  case ControlOffset:
    m_Control = static_cast<std::uint32_t>(inData);
    return true;
  default:
    return false;
  }
}

void MouseDevice::OnTick() {
  // Stateless
}

void MouseDevice::UpdateState(std::int32_t dx, std::int32_t dy,
                              std::uint8_t buttons) {
  // Apply Saturation Arithmetic for X
  std::int64_t newX = static_cast<std::int64_t>(m_AccX) + dx;
  if (newX > std::numeric_limits<std::int32_t>::max()) {
    m_AccX = std::numeric_limits<std::int32_t>::max();
    m_OverflowX = true;
  } else if (newX < std::numeric_limits<std::int32_t>::min()) {
    m_AccX = std::numeric_limits<std::int32_t>::min();
    m_OverflowX = true;
  } else {
    m_AccX = static_cast<std::int32_t>(newX);
  }

  // Apply Saturation Arithmetic for Y
  std::int64_t newY = static_cast<std::int64_t>(m_AccY) + dy;
  if (newY > std::numeric_limits<std::int32_t>::max()) {
    m_AccY = std::numeric_limits<std::int32_t>::max();
    m_OverflowY = true;
  } else if (newY < std::numeric_limits<std::int32_t>::min()) {
    m_AccY = std::numeric_limits<std::int32_t>::min();
    m_OverflowY = true;
  } else {
    m_AccY = static_cast<std::int32_t>(newY);
  }

  m_Buttons = buttons;

  bool irqEnabled = (m_Control & 1);
  if (irqEnabled && m_Pic) {
    m_Pic->RaiseIrq(PicDevice::IrqMouse);
  }
}

} // namespace Aurelia::Peripherals
