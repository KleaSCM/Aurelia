/**
 * Programmable Interrupt Controller (PIC) Device.
 *
 * Centralized Interrupt Request Management.
 *
 * The PIC aggregates interrupt requests from multiple peripheral devices,
 * applies enable masks, and signals the CPU when service is required.
 * This architecture mirrors classic interrupt controllers found in PC/AT
 * (Intel 8259), ARM GIC (Generic Interrupt Controller), and RISC-V PLIC
 * (Platform-Level Interrupt Controller).
 *
 * DESIGN PHILOSOPHY:
 * Modern systems use hierarchical interrupt routing with programmable
 * priorities, edge/level detection, and multi-core affinity. We implement
 * a simplified model suitable for single-core embedded systems while
 * maintaining industry-standard register semantics.
 *
 * REGISTER MAP:
 * ┌────────────┬─────────────────────────────────────────────────┐
 * │ Offset     │ Register                                        │
 * ├────────────┼─────────────────────────────────────────────────┤
 * │ 0x0000     │ IRQ_STATUS  - Pending Interrupt Status (RO)     │
 * │ 0x0004     │ IRQ_ENABLE  - Interrupt Enable Mask (RW)        │
 * │ 0x0008     │ IRQ_ACK     - Interrupt Acknowledge (W1C)       │
 * │ 0x000C     │ IRQ_TRIGGER - Edge vs Level Config (RW)         │
 * └────────────┴─────────────────────────────────────────────────┘
 *
 * IRQ_STATUS (Offset 0x0, Read-Only):
 * ┌───┬───┬───┬───┬───┬───┬───┬───┐
 * │15 │14 │13 │12 │11 │10 │ 9 │ 8 │ ... │ 1 │ 0 │
 * └───┴───┴───┴───┴───┴───┴───┴───┘     └───┴───┘
 *   │                                       │   └── IRQ 0: UART RX
 *   │                                       └────── IRQ 1: Timer
 *   └────────────────────────────────────────────── IRQ 15: Reserved
 *
 * Each bit represents one IRQ line:
 * - 0: No interrupt pending
 * - 1: Interrupt asserted by peripheral
 *
 * IRQ_ENABLE (Offset 0x4, Read-Write):
 * Mask register controlling which IRQs propagate to CPU.
 * - 0: IRQ masked (blocked)
 * - 1: IRQ enabled (can trigger CPU interrupt)
 *
 * IRQ_ACK (Offset 0x8, Write-1-to-Clear):
 * Software writes '1' to bit positions to acknowledge and clear
 * corresponding pending interrupts. Reading returns current status.
 * - Write 1: Clear pending IRQ
 * - Write 0: No effect
 *
 * IRQ_TRIGGER (Offset 0xC, Read-Write):
 * Configure interrupt sensitivity per line:
 * - 0: Level-triggered (asserted while condition active)
 * - 1: Edge-triggered (latched on 0→1 transition)
 *
 * INTERRUPT WORKFLOW:
 * 1. Peripheral asserts IRQ line (calls RaiseIrq())
 * 2. PIC sets corresponding bit in IRQ_STATUS
 * 3. If IRQ_ENABLE bit set, CPU IRQ line asserted
 * 4. CPU saves context, reads IRQ_STATUS to identify source
 * 5. CPU services interrupt, writes IRQ_ACK to clear
 * 6. PIC deasserts CPU IRQ line when no enabled IRQs pending
 *
 * PRIORITY RESOLUTION:
 * Lower-numbered IRQs have higher priority (IRQ 0 > IRQ 15).
 * CPU software scans IRQ_STATUS from LSB to MSB to service
 * highest-priority interrupt first.
 *
 * EDGE vs LEVEL SEMANTICS:
 * - Level: Signal held until source condition cleared AND acknowledged
 * - Edge: Latched once, cleared by acknowledge (source can de-assert)
 *
 * PHYSICAL EQUIVALENTS:
 * - Intel 8259 PIC: Cascadable, 8 IRQs per controller
 * - ARM GIC: 32-1020 IRQs, priority levels, CPU affinity
 * - RISC-V PLIC: Up to 1024 IRQs, priority threshold
 * - APIC (x86): Per-core local APIC + I/O APIC for routing
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#pragma once

#include "Bus/IBusDevice.hpp"
#include "Core/Types.hpp"
#include <cstdint>

namespace Aurelia::Peripherals {

class PicDevice final : public Bus::IBusDevice {
public:
  /**
   * @brief Construct PIC with all IRQs disabled.
   *
   * Initial state:
   * - IRQ_STATUS = 0 (no pending interrupts)
   * - IRQ_ENABLE = 0 (all masked)
   * - IRQ_TRIGGER = 0 (all level-triggered)
   */
  PicDevice();

  /**
   * IRQ LINE DEFINITIONS
   */
  static constexpr std::uint8_t MaxIrqLines = 16;
  static constexpr std::uint8_t IrqUartRx = 0;   // UART receive data
  static constexpr std::uint8_t IrqTimer = 1;    // Timer expire
  static constexpr std::uint8_t IrqKeyboard = 2; // Keyboard data ready
  static constexpr std::uint8_t IrqMouse = 3;    // Mouse movement/click

  /**
   * @brief Check if address falls within PIC register range.
   *
   * PIC occupies 4KB page starting at PicBase (0xE0002000).
   * Only first 16 bytes (4 registers) currently used.
   *
   * @param addr Physical address to check
   * @return true if addr in [PicBase, PicBase + 0xFFF]
   */
  [[nodiscard]] bool IsAddressInRange(Core::Address addr) const override;

  /**
   * @brief Handle read from PIC register.
   *
   * IRQ_STATUS (0x0): Return pending IRQ bits
   * IRQ_ENABLE (0x4): Return enabled IRQ mask
   * IRQ_ACK (0x8): Return current status (same as IRQ_STATUS)
   * IRQ_TRIGGER (0xC): Return trigger mode configuration
   *
   * @param addr Register address (offset from PicBase)
   * @param outData Reference to store read value
   * @return true if read successful
   */
  bool OnRead(Core::Address addr, Core::Data &outData) override;

  /**
   * @brief Handle write to PIC register.
   *
   * IRQ_STATUS (0x0): Read-only, writes ignored
   * IRQ_ENABLE (0x4): Update IRQ enable mask
   * IRQ_ACK (0x8): Clear pending IRQs (write-1-to-clear)
   * IRQ_TRIGGER (0xC): Configure edge vs level per IRQ
   *
   * @param addr Register address (offset from PicBase)
   * @param inData Value to write
   * @return true if write successful
   */
  bool OnWrite(Core::Address addr, Core::Data inData) override;

  /**
   * @brief Tick handler (stateless for PIC).
   *
   * PIC has no autonomous state transitions.
   * All state changes driven by external events (RaiseIrq, ClearIrq).
   */
  void OnTick() override;

  /**
   * @brief Assert IRQ line from peripheral.
   *
   * Called by peripheral device when interrupt condition occurs.
   * Sets corresponding bit in IRQ_STATUS register.
   *
   * LEVEL-TRIGGERED: Peripheral must hold assertion until serviced.
   * EDGE-TRIGGERED: Peripheral can de-assert immediately, IRQ latched.
   *
   * @param irqLine IRQ line number (0-15)
   */
  void RaiseIrq(std::uint8_t irqLine);

  /**
   * @brief De-assert IRQ line from peripheral.
   *
   * Called by peripheral when interrupt condition clears.
   * For level-triggered IRQs, removes pending status if software
   * has not yet acknowledged.
   *
   * @param irqLine IRQ line number (0-15)
   */
  void ClearIrq(std::uint8_t irqLine);

  /**
   * @brief Check if any enabled IRQ is pending.
   *
   * CPU polls this to determine if interrupt handling needed.
   * True if: (IRQ_STATUS & IRQ_ENABLE) != 0
   *
   * @return true if CPU should service interrupt
   */
  [[nodiscard]] bool HasPendingIrq() const;

  /**
   * @brief Get highest-priority pending IRQ number.
   *
   * Scans IRQ_STATUS & IRQ_ENABLE from LSB to MSB, returning
   * first set bit (lowest IRQ number = highest priority).
   *
   * @return IRQ number (0-15), or 0xFF if none pending
   */
  [[nodiscard]] std::uint8_t GetPendingIrqNumber() const;

private:
  /**
   * MEMORY MAP CONSTANTS
   */
  static constexpr Core::Address BaseAddr = 0xE0002000;
  static constexpr Core::Address RegisterBlockSize = 0x1000; // 4 KB

  static constexpr Core::Address IrqStatusOffset = 0x0;
  static constexpr Core::Address IrqEnableOffset = 0x4;
  static constexpr Core::Address IrqAckOffset = 0x8;
  static constexpr Core::Address IrqTriggerOffset = 0xC;

  /**
   * @brief Pending interrupt status.
   *
   * Bit N set when IRQ N asserted by peripheral.
   * Cleared by software acknowledge or peripheral ClearIrq().
   */
  std::uint16_t m_IrqStatus = 0;

  /**
   * @brief Interrupt enable mask.
   *
   * Bit N set when IRQ N allowed to reach CPU.
   * Software control for masking unwanted interrupts.
   */
  std::uint16_t m_IrqEnable = 0;

  /**
   * @brief Trigger mode configuration.
   *
   * Bit N = 0: Level-triggered (default)
   * Bit N = 1: Edge-triggered
   *
   * NOTE (KleaSCM): Edge-triggered IRQs require latching logic
   * to capture transient signals. Our implementation latches
   * on RaiseIrq() and holds until software acknowledges.
   */
  std::uint16_t m_IrqTrigger = 0;
};

} // namespace Aurelia::Peripherals
