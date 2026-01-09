/**
 * UART (Universal Asynchronous Receiver/Transmitter) Device.
 *
 * Memory-Mapped Serial Console Interface.
 *
 * This implementation provides a simplified UART controller mapped to
 * system memory, enabling character-based I/O through MMIO registers.
 * In a physical system, UART converts parallel data to/from serial streams
 * with configurable baud rates, parity, and stop bits. Here, we abstract
 * the physical layer and provide direct access to the host's stdin/stdout.
 *
 * REGISTER MAP:
 * ┌────────────┬─────────────────────────────────────────────────┐
 * │ Offset     │ Register                                        │
 * ├────────────┼─────────────────────────────────────────────────┤
 * │ 0x0000     │ DATA   - Transmit/Receive Data Register         │
 * │ 0x0004     │ STATUS - Status and Flag Register (Read-Only)   │
 * │ 0x0008     │ CONTROL - Control and Configuration Register    │
 * └────────────┴─────────────────────────────────────────────────┘
 *
 * DATA REGISTER (Offset 0x0):
 * - Write: Transmit byte to UART TX buffer (stdout)
 * - Read: Receive byte from UART RX buffer (stdin)
 * - Only bits [7:0] are significant; upper bits ignored
 *
 * STATUS REGISTER (Offset 0x4, Read-Only):
 * ┌───┬───┬───┬───┬───┬───┬───┬───┐
 * │ 7 │ 6 │ 5 │ 4 │ 3 │ 2 │ 1 │ 0 │
 * └───┴───┴───┴───┴───┴───┴───┴───┘
 *   │   │   │   │   │   │   │   └── TX_READY: Transmitter ready (always 1)
 *   │   │   │   │   │   │   └────── RX_AVAIL: Data available in RX buffer
 *   │   │   │   │   │   └────────── Reserved
 *   │   │   │   │   └────────────── Reserved
 *   │   │   │   └────────────────── Reserved
 *   │   │   └────────────────────── Reserved
 *   │   └────────────────────────── Reserved
 *   └────────────────────────────── Reserved
 *
 * CONTROL REGISTER (Offset 0x8):
 * ┌───┬───┬───┬───┬───┬───┬───┬───┐
 * │ 7 │ 6 │ 5 │ 4 │ 3 │ 2 │ 1 │ 0 │
 * └───┴───┴───┴───┴───┴───┴───┴───┘
 *   │   │   │   │   │   │   │   └── Reserved
 *   │   │   │   │   │   │   └────── Reserved
 *   │   │   │   │   │   └────────── TX_IRQ_EN: Enable TX interrupt
 *   │   │   │   │   └────────────── RX_IRQ_EN: Enable RX interrupt
 *   │   │   │   └────────────────── Reserved
 *   │   │   └────────────────────── Reserved
 *   │   └────────────────────────── Reserved
 *   └────────────────────────────── Reserved
 *
 * INTERRUPT BEHAVIOR:
 * - TX IRQ: Fires when transmitter becomes ready (always ready in this impl)
 * - RX IRQ: Fires when data arrives in receive buffer
 * - IRQ line connects to PIC for system-level interrupt handling
 *
 * IMPLEMENTATION NOTES:
 * - Baud rate simulation omitted (instant transmission)
 * - No hardware FIFO (single-byte buffer)
 * - Non-blocking reads return 0x00 if no data available
 * - Writes to stdout are immediate (no buffering delay)
 *
 * PHYSICAL UART EQUIVALENTS:
 * - 16550 UART: Industry-standard PC serial controller
 * - PL011 (ARM): ARM AMBA UART with FIFO
 * - NS16C550: Enhanced version with 16-byte FIFO
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#pragma once

#include "Bus/IBusDevice.hpp"
#include "Core/Types.hpp"
#include <cstdint>
#include <queue>

namespace Aurelia::Peripherals {

class UartDevice final : public Bus::IBusDevice {
public:
  /**
   * @brief Construct UART device.
   *
   * Initializes UART with empty RX buffer, TX ready, and IRQs disabled.
   * Base address is fixed at MemoryMap::UartBase (0xE0001000).
   */
  UartDevice();

  /**
   * @brief Check if address falls within UART register range.
   *
   * UART occupies a 4KB page starting at UartBase for future expansion.
   * Currently only 12 bytes (3 registers * 4 bytes) are used.
   *
   * @param addr Physical address to check
   * @return true if addr in [UartBase, UartBase + 0xFFF]
   */
  [[nodiscard]] bool IsAddressInRange(Core::Address addr) const override;

  /**
   * @brief Handle read from UART register.
   *
   * DATA (0x0): Pop byte from RX buffer, return 0x00 if empty
   * STATUS (0x4): Return status flags (TX_READY | RX_AVAIL)
   * CONTROL (0x8): Return current control register value
   *
   * @param addr Register address (offset from UartBase)
   * @param outData Reference to store read value
   * @return true if read successful, false if invalid register
   */
  bool OnRead(Core::Address addr, Core::Data &outData) override;

  /**
   * @brief Handle write to UART register.
   *
   * DATA (0x0): Transmit byte to stdout
   * STATUS (0x4): Read-only, writes ignored
   * CONTROL (0x8): Update TX/RX IRQ enable bits
   *
   * @param addr Register address (offset from UartBase)
   * @param inData Value to write (only lower 8 bits for DATA)
   * @return true if write successful, false if invalid register
   */
  bool OnWrite(Core::Address addr, Core::Data inData) override;

  /**
   * @brief Tick handler for UART state machine.
   *
   * UART is stateless in this implementation (no baud rate simulation).
   * This method is a no-op but required by IBusDevice interface.
   * Future: Could implement FIFO drain delays here.
   */
  void OnTick() override;

  /**
   * @brief Check if IRQ is pending.
   *
   * IRQ asserted when:
   * - RX data available AND RX_IRQ_EN set
   * - TX ready AND TX_IRQ_EN set (always ready, so fires immediately)
   *
   * @return true if interrupt should be signaled to PIC
   */
  [[nodiscard]] bool HasIrq() const { return m_IrqPending; }

  /**
   * @brief Clear pending IRQ.
   *
   * Called by PIC after CPU acknowledges interrupt.
   * Does not disable IRQ source - caller must clear condition or
   * disable IRQ in CONTROL register.
   */
  void ClearIrq() { m_IrqPending = false; }

  /**
   * @brief Simulate incoming data (for testing).
   *
   * Pushes byte into RX buffer and triggers RX IRQ if enabled.
   * In real hardware, this would be driven by serial line input.
   *
   * @param data Byte to place in RX buffer
   */
  void SimulateReceive(std::uint8_t data);

private:
  /**
   * MEMORY MAP CONSTANTS
   */
  static constexpr Core::Address BaseAddr = 0xE0001000;
  static constexpr Core::Address RegisterBlockSize = 0x1000; // 4 KB

  static constexpr Core::Address DataRegOffset = 0x0;
  static constexpr Core::Address StatusRegOffset = 0x4;
  static constexpr Core::Address ControlRegOffset = 0x8;

  /**
   * STATUS REGISTER BIT POSITIONS
   */
  static constexpr std::uint8_t StatusTxReady = 0; // Bit 0
  static constexpr std::uint8_t StatusRxAvail = 1; // Bit 1

  /**
   * CONTROL REGISTER BIT POSITIONS
   */
  static constexpr std::uint8_t ControlTxIrqEn = 2; // Bit 2
  static constexpr std::uint8_t ControlRxIrqEn = 3; // Bit 3

  /**
   * @brief Receive buffer.
   *
   * In physical UART: FIFO with depth 1-16 bytes.
   * Here: Queue simulating infinite buffer (limited by memory).
   *
   * NOTE (KleaSCM) std::queue uses std::deque internally, which is
   * inefficient for single-byte storage but acceptable for simulation.
   * Production UART would use circular buffer with fixed size.
   */
  std::queue<std::uint8_t> m_RxBuffer;

  /**
   * @brief Control register state.
   *
   * Stores IRQ enable bits and future configuration flags.
   * Only bits [3:2] currently used.
   */
  std::uint8_t m_Control = 0;

  /**
   * @brief IRQ pending flag.
   *
   * Set when interrupt condition met (RX data + enabled, or TX ready +
   * enabled). Cleared by external call to ClearIrq() after CPU services
   * interrupt.
   */
  bool m_IrqPending = false;

  /**
   * @brief Update IRQ pending state based on current conditions.
   *
   * Evaluates:
   * - RX_AVAIL && RX_IRQ_EN → assert IRQ
   * - TX_READY && TX_IRQ_EN → assert IRQ (TX always ready)
   *
   * Called after RX buffer changes or CONTROL register write.
   */
  void UpdateIrqState();
};

} // namespace Aurelia::Peripherals
