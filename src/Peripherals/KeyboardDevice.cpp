/**
 * Aurelia Keyboard Controller (KBC) Implementation.
 *
 * Implements the circular buffer logic and interrupt generation for the KBC.
 *
 * ALGORITHM:
 * The FIFO is implemented as a fixed-size ring buffer using Read/Write head
 * indices.
 * - Write: (Head + 1) % Size. Fails if Count == Size.
 * - Read: (Tail + 1) % Size. Fails if Count == 0.
 *
 * INTERRUPT LOGIC:
 * IRQ 2 is asserted whenever a new key is successfully enqueued and the
 * IRQ_ENABLE bit in the Control Register is set.
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#include "Peripherals/KeyboardDevice.hpp"

namespace Aurelia::Peripherals {

KeyboardDevice::KeyboardDevice() {
  // Initialize buffer to zero (security/cleanliness)
  m_Buffer.fill(0);
}

void KeyboardDevice::ConnectPic(PicDevice *pic) { m_Pic = pic; }

bool KeyboardDevice::IsAddressInRange(Core::Address addr) const {
  return addr >= BaseAddr && addr < (BaseAddr + 0x1000);
}

bool KeyboardDevice::OnRead(Core::Address addr, Core::Data &outData) {
  Core::Address offset = addr - BaseAddr;

  switch (offset) {
  case StatusOffset: {
    // Dynamically calculate status flags
    std::uint32_t status = 0;
    if (m_Count > 0) {
      status |= StatusRxReady;
    }
    if (m_Count == FifoSize) {
      status |= StatusFifoFull;
    }
    if (m_Overrun) {
      status |= StatusOverrun;
    }
    // Parity Error not simulated yet
    outData = status;
    return true;
  }
  case DataOffset: {
    if (m_Count == 0) {
      // Buffer Underflow: Return 0
      outData = 0;
      return true;
    }

    // Pop from FIFO
    outData = m_Buffer[m_ReadHead];
    m_ReadHead = (m_ReadHead + 1) % FifoSize;
    m_Count--;

    // Note: We do NOT clear IRQ here. IRQ is cleared by PIC Acknowledge.
    // However, if we empty the buffer, the "Cause" of the IRQ is gone.
    // Clear Overrun flag on successful read of data (Hardware behavior)
    m_Overrun = false;
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

bool KeyboardDevice::OnWrite(Core::Address addr, Core::Data inData) {
  Core::Address offset = addr - BaseAddr;

  switch (offset) {
  case ControlOffset:
    m_Control = static_cast<std::uint32_t>(inData);
    return true;
  default:
    // Status and Data are Read-Only
    return false;
  }
}

void KeyboardDevice::OnTick() {
  // Stateless device
}

void KeyboardDevice::EnqueueKey(std::uint8_t key) {
  if (m_Count == FifoSize) {
    // Buffer Overflow: Set Overrun Flag and Assert IRQ
    // This allows the OS to detect that it missed keystrokes.
    m_Overrun = true;

    // Trigger IRQ even if full, to ensure OS wakes up to drain it (or see
    // error)
    bool irqEnabled = (m_Control & 1);
    if (irqEnabled && m_Pic) {
      m_Pic->RaiseIrq(PicDevice::IrqKeyboard);
    }
    return;
  }

  // Push to FIFO
  m_Buffer[m_WriteHead] = key;
  m_WriteHead = (m_WriteHead + 1) % FifoSize;
  m_Count++;

  // Assert Interrupt if Enabled
  bool irqEnabled = (m_Control & 1);
  if (irqEnabled && m_Pic) {
    m_Pic->RaiseIrq(PicDevice::IrqKeyboard);
  }
}

} // namespace Aurelia::Peripherals
