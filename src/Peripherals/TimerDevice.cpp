/**
 * System Timer Implementation.
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#include "Peripherals/TimerDevice.hpp"
#include "Core/BitManip.hpp"

namespace Aurelia::Peripherals {

TimerDevice::TimerDevice() {
  /**
   * INITIALIZATION
   *
   * Reset to known state:
   * - Counter stopped (Control = 0)
   * - Counter cleared
   * - No pending interrupts
   */
  Counter = 0;
  Compare = 0;
  Control = 0;
  IrqPending = false;
}

bool TimerDevice::IsAddressInRange(Core::Address Address) const {
  return Address >= BaseAddr && Address < (BaseAddr + RegisterBlockSize);
}

bool TimerDevice::OnRead(Core::Address Address, Core::Data &OutData) {
  /**
   * REGISTER READ HANDLER
   *
   * Supports 64-bit atomic reads (since Data bus is 64-bit).
   */
  if (!IsAddressInRange(Address)) {
    return false;
  }

  Core::Address offset = Address - BaseAddr;

  switch (offset) {
  case CounterOffset:
    /**
     * COUNTER READ
     * Returns the live 64-bit counter value.
     */
    OutData = Counter;
    return true;

  case CompareOffset:
    /**
     * COMPARE READ
     * Returns the configured match target.
     */
    OutData = Compare;
    return true;

  case ControlOffset:
    /**
     * CONTROL READ
     * Returns configuration flags.
     */
    OutData = Control;
    return true;

  default:
    // RAZ (Read-As-Zero) for undefined
    OutData = 0;
    return true;
  }
}

bool TimerDevice::OnWrite(Core::Address Address, Core::Data InData) {
  /**
   * REGISTER WRITE HANDLER
   */
  if (!IsAddressInRange(Address)) {
    return false;
  }

  Core::Address offset = Address - BaseAddr;

  switch (offset) {
  case CounterOffset:
    /**
     * COUNTER WRITE
     * Read-Only register. Writes are ignored.
     *
     * NOTE (KleaSCM): Some timers allow writing to reset,
     * but we enforce use of Auto-Reset or Control disable/clear.
     */
    return true;

  case CompareOffset:
    /**
     * COMPARE WRITE
     * Sets the target value for the next interrupt.
     * If Counter > New Compare, it will wrap around before firing
     * (unless manually reset).
     */
    Compare = InData;
    return true;

  case ControlOffset:
    /**
     * CONTROL WRITE
     * Update Enable, IRQ Enable, and Reset modes.
     */
    Control = InData;
    return true;

  default:
    return true;
  }
}

void TimerDevice::OnTick() {
  /**
   * TICK HANDLER
   *
   * Core timing logic. Run every system clock cycle.
   */

  // 1. Check if Enabled
  bool isEnabled = Core::CheckBit(Control, ControlEnable);
  if (!isEnabled) {
    return;
  }

  // 2. Increment Counter
  Counter++;

  // 3. Check Match Condition
  if (Counter == Compare) {
    /**
     * COMPARE MATCH
     *
     * - Assert IRQ if enabled
     * - Handle Auto-Reset
     */
    bool irqEnabled = Core::CheckBit(Control, ControlIrqEn);
    if (irqEnabled) {
      IrqPending = true;
    }

    bool autoReset = Core::CheckBit(Control, ControlAutoReset);
    if (autoReset) {
      Counter = 0;
    }
  } else if (Counter > Compare) {
    // NOTE (KleaSCM) Missed match case (simplification):
    // If Counter was somehow set past Compare, it continues until overflow.
  }
}

} // namespace Aurelia::Peripherals
