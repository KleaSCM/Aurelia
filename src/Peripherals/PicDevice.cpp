/**
 * PIC Device Implementation.
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#include <bit>

#include "Core/BitManip.hpp"
#include "Peripherals/PicDevice.hpp"

namespace Aurelia::Peripherals {

PicDevice::PicDevice() {
  /**
   * POWER-ON INITIALIZATION
   *
   * PIC resets to safe state with all interrupts masked:
   * - No pending IRQs
   * - All IRQ lines disabled
   * - All configured as level-triggered
   *
   * Software must explicitly enable IRQs after configuring handlers.
   * This prevents spurious interrupts during boot sequence.
   */
  m_IrqStatus = 0;
  m_IrqEnable = 0;
  m_IrqTrigger = 0;
}

bool PicDevice::IsAddressInRange(Core::Address addr) const {
  /**
   * ADDRESS DECODING
   *
   * PIC occupies 4KB page for future expansion:
   * Base: 0xE0002000
   * End:  0xE0002FFF (inclusive)
   *
   * Current usage: 16 bytes (4 registers × 4 bytes)
   * Reserved space for:
   * - Per-IRQ priority levels
   * - CPU affinity (multi-core routing)
   * - IRQ pending edge detection timestamps
   * - Interrupt statistics counters
   */
  return addr >= BaseAddr && addr < (BaseAddr + RegisterBlockSize);
}

bool PicDevice::OnRead(Core::Address addr, Core::Data &outData) {
  /**
   * REGISTER READ HANDLER
   *
   * All reads complete in single cycle (no wait states).
   * Register access is atomic - no race conditions.
   */

  if (!IsAddressInRange(addr)) {
    return false; // Bus error
  }

  Core::Address offset = addr - BaseAddr;

  switch (offset) {
  case IrqStatusOffset: {
    /**
     * IRQ_STATUS READ
     *
     * Returns bitmap of pending interrupts.
     * Bit N set if IRQ N asserted by peripheral.
     *
     * CPU software typically reads this to identify
     * which interrupt source needs servicing.
     *
     * USAGE PATTERN:
     * 1. CPU receives interrupt signal
     * 2. Read IRQ_STATUS to identify source
     * 3. Service highest-priority IRQ (lowest bit number)
     * 4. Write IRQ_ACK to clear
     * 5. Repeat if multiple IRQs pending
     */
    outData = static_cast<Core::Data>(m_IrqStatus);
    return true;
  }

  case IrqEnableOffset: {
    /**
     * IRQ_ENABLE READ
     *
     * Returns current interrupt mask.
     * Software can read-modify-write to toggle specific IRQs.
     *
     * COMMON OPERATIONS:
     * - Read-modify-write: Enable single IRQ without affecting others
     * - Save/restore: Disable IRQs during critical section
     * - Debug: Query which IRQs are active
     */
    outData = static_cast<Core::Data>(m_IrqEnable);
    return true;
  }

  case IrqAckOffset: {
    /**
     * IRQ_ACK READ
     *
     * Reading ACK register returns current IRQ_STATUS.
     * This allows atomic read-acknowledge pattern.
     *
     * ALTERNATIVE DESIGN:
     * Some PICs return 0 on ACK read. We mirror STATUS
     * for convenience (software can read-acknowledge in sequence).
     */
    outData = static_cast<Core::Data>(m_IrqStatus);
    return true;
  }

  case IrqTriggerOffset: {
    /**
     * IRQ_TRIGGER READ
     *
     * Returns trigger mode configuration.
     * Bit N = 0: Level, Bit N = 1: Edge
     */
    outData = static_cast<Core::Data>(m_IrqTrigger);
    return true;
  }

  default:
    /**
     * RESERVED REGISTER READ
     *
     * Undefined offsets read as zero (forward compatibility).
     */
    outData = 0;
    return true;
  }
}

bool PicDevice::OnWrite(Core::Address addr, Core::Data inData) {
  /**
   * REGISTER WRITE HANDLER
   *
   * Processes configuration changes and interrupt acknowledgments.
   */

  if (!IsAddressInRange(addr)) {
    return false; // Bus error
  }

  Core::Address offset = addr - BaseAddr;

  switch (offset) {
  case IrqStatusOffset: {
    /**
     * IRQ_STATUS WRITE
     *
     * Status register is read-only.
     * IRQs set by peripheral RaiseIrq(), cleared by IRQ_ACK write.
     *
     * DESIGN RATIONALE:
     * Preventing software from directly manipulating IRQ_STATUS
     * ensures hardware/peripheral state matches interrupt state.
     * Software cannot "fake" interrupts or clear them incorrectly.
     */
    return true; // Silently ignore (read-only)
  }

  case IrqEnableOffset: {
    /**
     * IRQ_ENABLE WRITE
     *
     * Update interrupt enable mask.
     * Only lower 16 bits significant (16 IRQ lines).
     *
     * CRITICAL SECTION PATTERN:
     * 1. Software disables IRQs: Write 0x0000
     * 2. Execute critical code
     * 3. Restore IRQs: Write saved mask
     *
     * SELECTIVE MASKING:
     * To disable IRQ N: mask &= ~(1 << N)
     * To enable IRQ N:  mask |= (1 << N)
     */
    m_IrqEnable = static_cast<std::uint16_t>(inData & 0xFFFF);
    return true;
  }

  case IrqAckOffset: {
    /**
     * IRQ_ACK WRITE (Write-1-to-Clear)
     *
     * W1C SEMANTICS:
     * - Writing 1 to bit N: Clears IRQ N in STATUS register
     * - Writing 0 to bit N: No effect on that IRQ
     *
     * This allows clearing multiple IRQs in single write:
     * Write 0x0003 → Clear IRQ 0 and IRQ 1
     *
     * EDGE-TRIGGERED BEHAVIOR:
     * Edge IRQs always clear on acknowledge.
     *
     * LEVEL-TRIGGERED BEHAVIOR:
     * Level IRQs clear only if peripheral has de-asserted.
     * If signal still active, IRQ re-asserts immediately.
     * This models physical level-sensitive input.
     *
     * NOTE (KleaSCM): Our implementation latches level IRQs,
     * so acknowledge always clears. For true level behavior,
     * peripheral must call RaiseIrq() each tick while active.
     */
    std::uint16_t ackMask = static_cast<std::uint16_t>(inData & 0xFFFF);
    m_IrqStatus &= ~ackMask; // Clear acknowledged bits
    return true;
  }

  case IrqTriggerOffset: {
    /**
     * IRQ_TRIGGER WRITE
     *
     * Configure edge vs level sensitivity per IRQ line.
     *
     * SOFTWARE SETUP:
     * Typically configured once during initialization:
     * - UART RX: Edge (data arrival is transient event)
     * - Timer: Edge (countdown expire is discrete event)
     * - GPIO: Level (button held = continuous signal)
     *
     * CHANGING MODES:
     * Switching trigger mode on active IRQ is undefined.
     * Software should disable IRQ before mode change.
     */
    m_IrqTrigger = static_cast<std::uint16_t>(inData & 0xFFFF);
    return true;
  }

  default:
    /**
     * RESERVED REGISTER WRITE
     *
     * Writes to undefined offsets ignored (no side effects).
     */
    return true;
  }
}

void PicDevice::OnTick() {
  /**
   * TICK HANDLER (STATELESS)
   *
   * PIC is purely reactive - state changes only on:
   * - Peripheral RaiseIrq()/ClearIrq()
   * - Software register writes
   *
   * No autonomous behavior, so OnTick is no-op.
   *
   * FUTURE ENHANCEMENTS:
   * - Periodic IRQ status polling from peripherals
   * - Auto-clear timeout for stuck interrupts
   * - Interrupt coalescing (rate limiting)
   */
}

void PicDevice::RaiseIrq(std::uint8_t irqLine) {
  /**
   * PERIPHERAL IRQ ASSERTION
   *
   * Called by peripheral device when interrupt condition occurs.
   *
   * EDGE-TRIGGERED:
   * - Latch on 0→1 transition
   * - Peripheral can immediately de-assert
   * - IRQ remains pending until software acknowledges
   *
   * LEVEL-TRIGGERED:
   * - Set pending while signal active
   * - Peripheral must hold assertion until serviced
   * - IRQ clears when peripheral de-asserts AND software acknowledges
   *
   * IMPLEMENTATION NOTES:
   * Our model always latches - both edge and level IRQs sticky.
   * True level behavior requires peripheral to continuously
   * call RaiseIrq() while condition active. On acknowledge,
   * peripheral must stop calling RaiseIrq() to prevent re-assertion.
   */

  if (irqLine >= MaxIrqLines) {
    // Invalid IRQ line number
    return;
  }

  // Set corresponding bit in status register
  m_IrqStatus = Core::SetBit(m_IrqStatus, irqLine);
}

void PicDevice::ClearIrq(std::uint8_t irqLine) {
  /**
   * PERIPHERAL IRQ DE-ASSERTION
   *
   * Called by peripheral when interrupt condition clears.
   *
   * EDGE-TRIGGERED:
   * - No effect (edge IRQ latched until software acknowledges)
   *
   * LEVEL-TRIGGERED:
   * - Clear pending status
   * - If software has not yet acknowledged, IRQ disappears
   * - If software already processing, prevents re-assertion
   *
   * USAGE:
   * UART RX: ClearIrq() when buffer drained
   * Timer: ClearIrq() after counter reloaded
   * GPIO: ClearIrq() when pin returns to inactive state
   */

  if (irqLine >= MaxIrqLines) {
    return;
  }

  bool isEdgeTriggered = Core::CheckBit(m_IrqTrigger, irqLine);

  if (isEdgeTriggered) {
    /**
     * EDGE MODE:
     * Peripheral de-assertion has no effect.
     * IRQ remains latched until software acknowledges.
     */
    return;
  } else {
    /**
     * LEVEL MODE:
     * Clear IRQ if peripheral de-asserts signal.
     */
    m_IrqStatus = Core::ClearBit(m_IrqStatus, irqLine);
  }
}

bool PicDevice::HasPendingIrq() const {
  /**
   * CPU IRQ LINE EVALUATION
   *
   * Determines if CPU interrupt signal should be asserted.
   *
   * LOGIC:
   * CPU IRQ = (IRQ_STATUS & IRQ_ENABLE) != 0
   *
   * Any pending interrupt that is also enabled triggers CPU.
   * Masked interrupts do not propagate.
   *
   * CPU POLLING:
   * CPU checks HasPendingIrq() after each instruction.
   * If true, CPU enters interrupt service routine.
   */
  return (m_IrqStatus & m_IrqEnable) != 0;
}

std::uint8_t PicDevice::GetPendingIrqNumber() const {
  /**
   * PRIORITY RESOLUTION
   *
   * Identifies highest-priority pending interrupt.
   * Lower IRQ numbers have higher priority (IRQ 0 > IRQ 1 > ...).
   *
   * ALGORITHM:
   * Compute active IRQs: status & enable
   * Find first set bit (LSB → MSB scan)
   * Return bit position as IRQ number
   *
   * HARDWARE EQUIVALENT:
   * Priority encoders in physical PICs use combinational logic
   * for instant resolution. We use software bit scan.
   *
   * OPTIMIZATION:
   * std::countr_zero() maps to BSF (x86) or CLZ (ARM) instruction.
   * Constant-time operation on most architectures.
   */

  std::uint16_t activeIrqs = m_IrqStatus & m_IrqEnable;

  if (activeIrqs == 0) {
    return 0xFF; // No pending IRQs
  }

  // Find position of least significant set bit
  std::uint8_t irqNumber =
      static_cast<std::uint8_t>(std::countr_zero(activeIrqs));

  return irqNumber;
}

} // namespace Aurelia::Peripherals
