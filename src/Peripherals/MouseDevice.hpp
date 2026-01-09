/**
 * Aurelia Mouse Controller.
 *
 * Relative Pointing Device Interface.
 *
 * The Mouse Controller aggregates relative motion events (Delta X, Delta Y)
 * from a physical pointing device. Unlike absolute positioning devices
 * (like Touchscreens), mice report movement *changes*.
 *
 * DESIGN PHILOSOPHY:
 * We employ a Hardware Accumulator model. Motion events from the host OS
 * (which may arrive at >1000Hz) are summed into internal X/Y registers.
 * When the guest CPU reads these registers, the accumulation is atomically
 * reset to zero. This ensures no movement is lost even if the guest OS
 * polls at a lower frequency (e.g., 60Hz).
 *
 * REGISTER MAP:
 * ┌────────────┬─────────────────────────────────────────────────┐
 * │ Offset     │ Register                                        │
 * ├────────────┼─────────────────────────────────────────────────┤
 * │ 0x0000     │ STATUS   - Event State (RO)                     │
 * │ 0x0004     │ DATA_X   - Signed 32-bit X Delta (RO/Clear)     │
 * │ 0x0008     │ DATA_Y   - Signed 32-bit Y Delta (RO/Clear)     │
 * │ 0x000C     │ BUTTONS  - Button Bitmask (RO)                  │
 * │ 0x0010     │ CONTROL  - Interrupt Enable (RW)                │
 * └────────────┴─────────────────────────────────────────────────┘
 *
 * DATA REGISTERS (0x4, 0x8):
 * - Contain signed 32-bit integers representing accumulated movement.
 * - Reading a Data register returns the value and RESETS it to 0.
 *
 * BUTTONS REGISTER (0xC):
 * - Bit 0: Left Button
 * - Bit 1: Right Button
 * - Bit 2: Middle Button
 *
 * CONTROL REGISTER (0x10):
 * - Bit 0: IRQ_ENABLE. Assert IRQ 3 on any state change.
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#pragma once

#include "Bus/IBusDevice.hpp"
#include "Peripherals/PicDevice.hpp"
#include <cstdint>

namespace Aurelia::Peripherals {

class MouseDevice final : public Bus::IBusDevice {
public:
  MouseDevice();

  /**
   * @brief Connect weak reference to PIC for Interrupts.
   *
   * @param pic Pointer to the system Interrupt Controller.
   */
  void ConnectPic(PicDevice *pic);

  /**
   * @brief Check if address falls within Mouse register range.
   * @param addr Physical address to check
   * @return true if addr matches Mouse range (0xE0005000)
   */
  [[nodiscard]] bool IsAddressInRange(Core::Address addr) const override;

  /**
   * @brief Handle read from Mouse register.
   *
   * - DATA_X/Y: Returns delta and clears accumulator.
   * - BUTTONS: Returns current button state.
   *
   * @param addr Register address.
   * @param outData Reference to store read value.
   * @return true if read successful.
   */
  bool OnRead(Core::Address addr, Core::Data &outData) override;

  /**
   * @brief Handle write to Mouse register.
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
   */
  void OnTick() override;

  /**
   * @brief Inject Mouse Event from Host.
   *
   * Updates internal accumulators and triggers IRQ.
   *
   * @param dx Relative X movement (signed).
   * @param dy Relative Y movement (signed).
   * @param buttons Button bitmask (0-7).
   */
  void UpdateState(std::int32_t dx, std::int32_t dy, std::uint8_t buttons);

private:
  PicDevice *m_Pic = nullptr;

  /**
   * MEMORY MAP
   */
  static constexpr Core::Address BaseAddr = 0xE0005000;
  static constexpr Core::Address StatusOffset = 0x0;
  static constexpr Core::Address DataXOffset = 0x4;
  static constexpr Core::Address DataYOffset = 0x8;
  static constexpr Core::Address ButtonsOffset = 0xC;
  static constexpr Core::Address ControlOffset = 0x10;

  /**
   * STATUS FLAGS
   */
  static constexpr std::uint32_t StatusPacketReady = (1 << 0);
  static constexpr std::uint32_t StatusXOverflow = (1 << 1);
  static constexpr std::uint32_t StatusYOverflow = (1 << 2);

  /**
   * INTERNAL STATE (Accumulators)
   */
  std::int32_t m_AccX = 0;
  std::int32_t m_AccY = 0;
  bool m_OverflowX = false;
  bool m_OverflowY = false;
  std::uint8_t m_Buttons = 0;

  std::uint32_t m_Control = 0;
};

} // namespace Aurelia::Peripherals
