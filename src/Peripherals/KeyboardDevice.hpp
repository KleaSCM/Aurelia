/**
 * Aurelia Keyboard Controller (KBC).
 *
 * Buffered Character Input Device.
 *
 * The KBC manages asynchronous character input from the host system.
 * It employs a hardware FIFO (First-In-First-Out) buffer to decouple
 * high-speed typing bursts from the CPU's interrupt service latency.
 *
 * DESIGN PHILOSOPHY:
 * Direct polling of input devices causes missed keystrokes during high
 * CPU load. By implementing a ring buffer in hardware (simulated), we ensure
 * data integrity and allow the OS to process input in batches. This mirrors
 * the 8042 Keyboard Controller found in IBM PC systems.
 *
 * REGISTER MAP:
 * ┌────────────┬─────────────────────────────────────────────────┐
 * │ Offset     │ Register                                        │
 * ├────────────┼─────────────────────────────────────────────────┤
 * │ 0x0000     │ STATUS   - Buffer State & Flags (RO)            │
 * │ 0x0004     │ DATA     - Input Character FIFO (RO)            │
 * │ 0x0008     │ CONTROL  - Interrupt Enable (RW)                │
 * └────────────┴─────────────────────────────────────────────────┘
 *
 * STATUS REGISTER (0x0):
 * ┌───┬───┬───┬───┬───┬───┬───┬───┐
 * │ 7 │ 6 │ 5 │ 4 │ 3 │ 2 │ 1 │ 0 │
 * └───┴───┴───┴───┴───┴───┴───┴───┘
 *                           │   │   └── RX_READY: Data available
 *                           │   └────── PARITY_ERR: Integrity error
 *                           └────────── FIFO_FULL: Buffer overflow
 *
 * DATA REGISTER (0x4):
 * - Reads return the oldest character in the FIFO.
 * - Reading pops the character from the buffer.
 * - If Empty, reads return 0 and clear RX_READY.
 *
 * CONTROL REGISTER (0x8):
 * - Bit 0: IRQ_ENABLE. If set, asserts IRQ 2 when RX_READY transitions to high.
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#pragma once

#include "Bus/IBusDevice.hpp"
#include "Peripherals/PicDevice.hpp"
#include <array>
#include <cstdint>

namespace Aurelia::Peripherals {

class KeyboardDevice final : public Bus::IBusDevice {
public:
  /**
   * @brief Construct KBC with empty buffer.
   *
   * Initializes FIFO pointers and clears status flags.
   */
  KeyboardDevice();

  /**
   * @brief Connect weak reference to PIC for Interrupts.
   *
   * @param pic Pointer to the system Interrupt Controller.
   */
  void ConnectPic(PicDevice *pic);

  /**
   * @brief Check if address falls within KBC register range.
   *
   * Occupies 4KB page starting at dedicated Base.
   *
   * @param addr Physical address to check
   * @return true if addr matches KBC range
   */
  [[nodiscard]] bool IsAddressInRange(Core::Address addr) const override;

  /**
   * @brief Handle read from KBC register.
   *
   * - DATA: Pops from FIFO.
   * - STATUS: Returns flags.
   *
   * @param addr Register address.
   * @param outData Reference to store read value.
   * @return true if read successful.
   */
  bool OnRead(Core::Address addr, Core::Data &outData) override;

  /**
   * @brief Handle write to KBC register.
   *
   * - CONTROL: Update IRQ mask.
   *
   * @param addr Register address.
   * @param inData Value to write.
   * @return true if write successful.
   */
  bool OnWrite(Core::Address addr, Core::Data inData) override;

  /**
   * @brief Tick handler.
   *
   * KBC is event-driven; Tick is No-Op.
   */
  void OnTick() override;

  /**
   * @brief Enqueue a character from the Host System.
   *
   * This is the "Physical Interface" where the emulator injects real
   * keyboard events (e.g., from SDL2/ncurses).
   *
   * - Adds char to Ring Buffer.
   * - Sets RX_READY flag.
   * - Asserts IRQ 2 if enabled.
   *
   * @param key ASCII character code.
   */
  void EnqueueKey(std::uint8_t key);

private:
  PicDevice *m_Pic = nullptr;

  /**
   * MEMORY MAP CONSTANTS
   */
  static constexpr Core::Address BaseAddr = 0xE0004000;
  static constexpr Core::Address StatusOffset = 0x0;
  static constexpr Core::Address DataOffset = 0x4;
  static constexpr Core::Address ControlOffset = 0x8;

  /**
   * STATUS FLAGS
   */
  static constexpr std::uint32_t StatusRxReady = (1 << 0);
  static constexpr std::uint32_t StatusParityErr = (1 << 1);
  static constexpr std::uint32_t StatusFifoFull = (1 << 2);
  static constexpr std::uint32_t StatusOverrun = (1 << 3);

  /**
   * FIFO BUFFER
   * Fixed size circular buffer to prevent allocation during emulation.
   */
  static constexpr std::size_t FifoSize = 16;
  std::array<std::uint8_t, FifoSize> m_Buffer;
  std::size_t m_ReadHead = 0;
  std::size_t m_WriteHead = 0;
  std::size_t m_Count = 0;
  bool m_Overrun = false;

  std::uint32_t m_Control = 0; // Control Register
};

} // namespace Aurelia::Peripherals
