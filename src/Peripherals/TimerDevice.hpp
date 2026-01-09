/**
 * System Timer Device.
 *
 * Programmable Interval Timer (PIT).
 *
 * A simple 64-bit countdown/count-up timer that generates interrupts
 * at regular intervals. Essential for OS scheduling, timekeeping,
 * and periodic hardware events.
 *
 * REGISTER MAP:
 * ┌────────────┬─────────────────────────────────────────────────┐
 * │ Offset     │ Register                                        │
 * ├────────────┼─────────────────────────────────────────────────┤
 * │ 0x0000     │ COUNTER (Read-Only)                             │
 * │ 0x0008     │ COMPARE (Read-Write)                            │
 * │ 0x0010     │ CONTROL (Read-Write)                            │
 * └────────────┴─────────────────────────────────────────────────┘
 *
 * COUNTER (Offset 0x0, 64-bit):
 * - Current tick count.
 * - Increments on every system clock tick when enabled.
 * - Reset to 0 when matching COMPARE (if Auto-Reset enabled).
 *
 * COMPARE (Offset 0x8, 64-bit):
 * - Target tick count for interrupt generation.
 * - When COUNTER == COMPARE, IRQ is asserted.
 *
 * CONTROL (Offset 0x10, 8-bit):
 * ┌───┬───┬───┬───┬───┬───┬───┬───┐
 * │ 7 │...│ 3 │ 2 │ 1 │ 0 │
 * └───┴───┴───┴───┴───┴───┴───┴───┘
 *                   │   │   └── ENABLE: Start/Stop timer
 *                   │   └────── IRQ_EN: Enable Interrupts
 *                   └────────── AUTO_RESET: Reset counter on match
 *
 * INTERRUPT BEHAVIOR:
 * - Fires when COUNTER reaches COMPARE value.
 * - Cleared by writing to status/control or explicit acknowledge.
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#pragma once

#include <cstdint>

#include "Bus/IBusDevice.hpp"
#include "Core/Types.hpp"

namespace Aurelia::Peripherals {

class TimerDevice final : public Bus::IBusDevice {
public:
  /**
   * @brief Construct Timer Device.
   *
   * Initializes Timer with 0 counter and disabled state.
   * Base address defaults to MemoryMap::TimerBase (0xE0003000).
   */
  TimerDevice();

  /**
   * @brief Check if address falls within Timer register range.
   *
   * Timer occupies a 4KB page.
   *
   * @param Address Physical address to check
   * @return true if Address is valid for this device
   */
  [[nodiscard]] bool IsAddressInRange(Core::Address Address) const override;

  /**
   * @brief Handle read from Timer register.
   *
   * COUNTER (0x0): Returns current 64-bit count.
   * COMPARE (0x8): Returns current compare target.
   * CONTROL (0x10): Returns control flags.
   */
  bool OnRead(Core::Address Address, Core::Data &OutData) override;

  /**
   * @brief Handle write to Timer register.
   *
   * COUNTER (0x0): Read-only (writes ignored).
   * COMPARE (0x8): Update match target.
   * CONTROL (0x10): Update configuration (Enable, IRQ, Auto-Reset).
   */
  bool OnWrite(Core::Address Address, Core::Data InData) override;

  /**
   * @brief Tick handler.
   *
   * Increments counter if enabled.
   * Checks for Compare match and asserts IRQ.
   * Handles Auto-Reset logic.
   */
  void OnTick() override;

  /**
   * @brief Check if IRQ is pending.
   *
   * @return true if Timer has asserted interrupt
   */
  [[nodiscard]] bool HasIrq() const { return IrqPending; }

  /**
   * @brief Clear pending IRQ.
   */
  void ClearIrq() { IrqPending = false; }

private:
  /**
   * MEMORY MAP CONSTANTS
   */
  static constexpr Core::Address BaseAddr = 0xE0003000;
  static constexpr Core::Address RegisterBlockSize = 0x1000;

  static constexpr Core::Address CounterOffset = 0x00;
  static constexpr Core::Address CompareOffset = 0x08;
  static constexpr Core::Address ControlOffset = 0x10;

  /**
   * CONTROL BIT DEFINITIONS
   */
  static constexpr std::uint8_t ControlEnable = 0;    // Bit 0
  static constexpr std::uint8_t ControlIrqEn = 1;     // Bit 1
  static constexpr std::uint8_t ControlAutoReset = 2; // Bit 2

  /**
   * INTERNAL STATE
   */
  Core::Word Counter = 0;
  Core::Word Compare = 0;
  Core::Word Control = 0;

  bool IrqPending = false;
};

} // namespace Aurelia::Peripherals
