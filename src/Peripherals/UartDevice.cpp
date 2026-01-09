/**
 * UART Device Implementation.
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#include "Peripherals/UartDevice.hpp"
#include "Core/BitManip.hpp"
#include <iostream>

namespace Aurelia::Peripherals {

UartDevice::UartDevice() {
  /**
   * INITIALIZATION SEQUENCE
   *
   * UART powers on in a known state:
   * - TX ready (can transmit immediately)
   * - RX buffer empty
   * - All interrupts disabled
   * - No pending IRQs
   *
   * Physical UART would perform baud rate calibration,
   * line state detection, and FIFO reset here.
   */
  m_Control = 0;
  m_IrqPending = false;
  // m_RxBuffer default-constructed to empty queue
}

bool UartDevice::IsAddressInRange(Core::Address addr) const {
  /**
   * ADDRESS DECODING
   *
   * UART occupies 4KB page for future expansion:
   * Base: 0xE0001000
   * End:  0xE0001FFF (inclusive)
   *
   * Only first 12 bytes currently defined (3 regs × 4 bytes).
   * Remaining space reserved for:
   * - FIFO control registers
   * - Baud rate divisor
   * - Line control (parity, stop bits)
   * - Modem status/control
   */
  return addr >= BaseAddr && addr < (BaseAddr + RegisterBlockSize);
}

bool UartDevice::OnRead(Core::Address addr, Core::Data &outData) {
  /**
   * READ OPERATION HANDLER
   *
   * Determines which register is being accessed and returns
   * appropriate value. All reads are synchronous (no wait states).
   *
   * NOTE (KleaSCM): Real UART may stall on RX read if FIFO
   * is being drained by hardware state machine. We have no
   * such timing constraints in simulation.
   */

  if (!IsAddressInRange(addr)) {
    return false; // Bus error
  }

  Core::Address offset = addr - BaseAddr;

  switch (offset) {
  case DataRegOffset: {
    /**
     * DATA REGISTER READ (RX)
     *
     * Pop oldest byte from receive buffer.
     * If buffer empty, return 0x00 (null character).
     *
     * HARDWARE BEHAVIOR:
     * - Reading DATA clears RX_AVAIL flag when buffer empties
     * - May trigger RX_EMPTY interrupt if configured
     * - Hardware UART would shift next byte from FIFO
     */
    if (!m_RxBuffer.empty()) {
      outData = static_cast<Core::Data>(m_RxBuffer.front());
      m_RxBuffer.pop();

      // Update IRQ state after buffer change
      UpdateIrqState();
    } else {
      // No data available
      outData = 0x00;
    }
    return true;
  }

  case StatusRegOffset: {
    /**
     * STATUS REGISTER READ
     *
     * Bit 0 (TX_READY):  Always 1 (infinite TX buffer)
     * Bit 1 (RX_AVAIL):  1 if RX buffer non-empty
     * Bits [7:2]:        Reserved (read as 0)
     *
     * PHYSICAL UART STATUS BITS:
     * - Overrun error (data lost due to full FIFO)
     * - Parity error
     * - Framing error (invalid stop bit)
     * - Break detect (line held low)
     */
    Core::Data status = 0;

    // TX always ready (bit 0)
    status = Core::SetBit(status, StatusTxReady);

    // RX available if buffer not empty (bit 1)
    if (!m_RxBuffer.empty()) {
      status = Core::SetBit(status, StatusRxAvail);
    }

    outData = status;
    return true;
  }

  case ControlRegOffset: {
    /**
     * CONTROL REGISTER READ
     *
     * Returns current configuration state.
     * Software can read-modify-write to change specific bits.
     */
    outData = static_cast<Core::Data>(m_Control);
    return true;
  }

  default:
    /**
     * RESERVED REGISTER ACCESS
     *
     * Reads from undefined offsets return 0.
     * Alternative: Could return bus error (false).
     *
     * NOTE (KleaSCM): ARM spec recommends RAZ (read-as-zero)
     * for reserved registers to maintain forward compatibility.
     */
    outData = 0;
    return true;
  }
}

bool UartDevice::OnWrite(Core::Address addr, Core::Data inData) {
  /**
   * WRITE OPERATION HANDLER
   *
   * Processes writes to UART registers.
   * All writes complete in single cycle (no wait states).
   */

  if (!IsAddressInRange(addr)) {
    return false; // Bus error
  }

  Core::Address offset = addr - BaseAddr;

  switch (offset) {
  case DataRegOffset: {
    /**
     * DATA REGISTER WRITE (TX)
     *
     * Transmit byte to host stdout.
     * Only lower 8 bits used; upper bits ignored.
     *
     * HARDWARE BEHAVIOR:
     * - Byte placed in TX FIFO
     * - Hardware state machine shifts bits out serial line
     * - TX_READY cleared if FIFO becomes full
     * - TX_COMPLETE IRQ fires when transmission finishes
     *
     * SIMULATION:
     * - Direct write to stdout (no buffering)
     * - Character immediately visible to user
     * - No transmission delay
     */
    std::uint8_t txByte = static_cast<std::uint8_t>(inData & 0xFF);
    std::cout << static_cast<char>(txByte) << std::flush;

    // TX IRQ could fire here if TX_IRQ_EN set
    // (omitted since TX always succeeds immediately)

    return true;
  }

  case StatusRegOffset: {
    /**
     * STATUS REGISTER WRITE
     *
     * Status register is read-only. Writes have no effect.
     *
     * ALTERNATIVE DESIGNS:
     * - Some UARTs allow writing STATUS to clear error flags
     * - "Write-1-to-clear" pattern (W1C) for sticky bits
     *
     * We implement strict read-only semantics.
     */
    return true; // Silently ignore
  }

  case ControlRegOffset: {
    /**
     * CONTROL REGISTER WRITE
     *
     * Update IRQ enable bits and future configuration.
     * Only bits [3:2] currently writable:
     * - Bit 2: TX_IRQ_EN
     * - Bit 3: RX_IRQ_EN
     *
     * FUTURE EXTENSIONS:
     * - Baud rate select
     * - Parity enable/type
     * - Stop bit count
     * - Hardware flow control enable
     */
    m_Control = static_cast<std::uint8_t>(inData & 0xFF);

    // IRQ state may change based on new enable bits
    UpdateIrqState();

    return true;
  }

  default:
    /**
     * RESERVED REGISTER WRITE
     *
     * Writes to undefined offsets are ignored.
     * Alternative: Could return bus error.
     */
    return true; // Silently ignore
  }
}

void UartDevice::OnTick() {
  /**
   * TICK HANDLER (STATELESS)
   *
   * UART has no asynchronous state transitions in this implementation.
   * Physical UART would use OnTick for:
   * - Baud rate generation (clock divider countdown)
   * - Bit-level TX/RX state machine
   * - FIFO drain/fill operations
   * - Break detection timing
   *
   * Our implementation is purely reactive (register-driven),
   * so OnTick is a no-op.
   */
}

void UartDevice::SimulateReceive(std::uint8_t data) {
  /**
   * SIMULATED RECEIVE PATH
   *
   * In physical UART: Serial data arrives asynchronously,
   * hardware shifts bits into RX shift register, then moves
   * complete byte to FIFO when stop bit detected.
   *
   * Here: Direct injection into RX buffer for testing.
   *
   * USAGE:
   * Test code can call this to simulate keyboard input
   * or data arriving on serial line.
   *
   * @param data Byte to place in RX buffer
   */
  m_RxBuffer.push(data);

  // RX data now available - update IRQ if enabled
  UpdateIrqState();
}

void UartDevice::UpdateIrqState() {
  /**
   * INTERRUPT REQUEST STATE MACHINE
   *
   * Evaluates current conditions to determine if IRQ should be asserted:
   *
   * CONDITION 1: RX Data Available
   * - RX buffer not empty
   * - RX_IRQ_EN bit set in CONTROL
   *
   * CONDITION 2: TX Ready
   * - TX buffer has space (always true here)
   * - TX_IRQ_EN bit set in CONTROL
   *
   * IRQ remains asserted until:
   * - Source condition cleared (RX buffer drained, IRQ disabled)
   * - CPU explicitly clears via ClearIrq() after servicing
   *
   * EDGE vs LEVEL:
   * This implements level-triggered IRQ (signal held while condition true).
   * Alternative: Edge-triggered (pulse on 0→1 transition only).
   */

  bool rxIrqCondition =
      !m_RxBuffer.empty() && Core::CheckBit(m_Control, ControlRxIrqEn);
  bool txIrqCondition =
      Core::CheckBit(m_Control, ControlTxIrqEn); // TX always ready

  m_IrqPending = rxIrqCondition || txIrqCondition;
}

} // namespace Aurelia::Peripherals
