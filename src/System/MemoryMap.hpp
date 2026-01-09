/**
 * Aurelia System Memory Map.
 *
 * Defines the physical address space layout for the entire system.
 * This maps where each hardware component lives in the 64-bit address space.
 *
 * MEMORY MAP:
 * ┌──────────────────┬────────────────────────────────────────┐
 * │ Address Range    │ Component                              │
 * ├──────────────────┼────────────────────────────────────────┤
 * │ 0x0000_0000 -    │ RAM (Main Memory)                      │
 * │ 0x0FFF_FFFF      │ Size: 256 MB                           │
 * ├──────────────────┼────────────────────────────────────────┤
 * │ 0x1000_0000 -    │ Reserved (Future Expansion)            │
 * │ 0xDFFF_FFFF      │                                        │
 * ├──────────────────┼────────────────────────────────────────┤
 * │ 0xE000_0000 -    │ Storage Controller MMIO                │
 * │ 0xE000_0FFF      │ Size: 4 KB (NVMe-like registers)       │
 * ├──────────────────┼────────────────────────────────────────┤
 * │ 0xE000_1000 -    │ Reserved (Future: UART, PIC, Timer)    │
 * │ 0xFFFF_FFFF      │                                        │
 * └──────────────────┴────────────────────────────────────────┘
 *
 * DESIGN RATIONALE:
 * - RAM at 0x0: Simplifies reset vector (PC = 0x0 on boot)
 * - MMIO at high addresses: Standard practice, keeps low memory clean
 * - 256 MB RAM: Large enough for real programs, small enough to simulate
 * - 64-bit addressing: Future-proof, matches modern CPU designs
 *
 * USAGE:
 * When connecting devices to the Bus, use these constants to define
 * their address ranges:
 *
 *   bus.ConnectDevice(&ram, RamBase, RamEnd);
 *   bus.ConnectDevice(&storage, StorageControllerBase, StorageControllerEnd);
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#pragma once

#include "Core/Types.hpp"

namespace Aurelia::System {

/**
 * Type Aliases
 *
 * Import Core types into System namespace for convenience.
 */
using Address = Core::Address;

/**
 * RAM (Main Memory)
 *
 * Base Address: 0x0000_0000
 * Size: 256 MB (0x1000_0000 bytes)
 *
 * This is the primary working memory accessible to the CPU.
 * Programs are loaded here, stack lives here, heap allocations happen here.
 *
 * NOTE (KleaSCM) Starting RAM at address 0 simplifies the reset vector.
 * On power-on, PC = 0x0, which points to the first byte of RAM where
 * the bootloader/program resides.
 */
constexpr Address RamBase = 0x00000000;
constexpr std::size_t RamSize = 256 * 1024 * 1024; // 256 MB
constexpr Address RamEnd = RamBase + RamSize - 1;  // 0x0FFF_FFFF

/**
 * expansion.
 */
constexpr Address MmioBase = 0xE0000000;

/**
 * Storage Controller MMIO
 *
 * Base Address: 0xE000_0000
 * Size: 4 KB (0x1000 bytes)
 *
 * Maps the NVMe-like controller registers:
 * - CAP (Capabilities)
 * - CC (Controller Configuration)
 * - CSTS (Controller Status)
 * - Admin Queue Addresses
 * - Doorbell Registers
 *
 * See Storage/Controller/StorageController.hpp for register layout.
 */
constexpr Address StorageControllerBase = MmioBase;
constexpr std::size_t StorageControllerSize = 4096; // 4 KB
constexpr Address StorageControllerEnd =
    StorageControllerBase + StorageControllerSize - 1;

/**
 * Future Peripheral Regions (Phase 8)
 *
 * Reserved address space for upcoming I/O devices:
 * - UART (Serial Console): Will be at 0xE000_1000
 * - PIC (Interrupt Controller): Will be at 0xE000_2000
 * - Timer: Will be at 0xE000_3000
 */
constexpr Address UartBase = 0xE0001000;     // 4 KB reserved
constexpr Address PicBase = 0xE0002000;      // 4 KB reserved
constexpr Address TimerBase = 0xE0003000;    // 4 KB reserved
constexpr Address KeyboardBase = 0xE0004000; // 4 KB reserved
constexpr Address MouseBase = 0xE0005000;    // 4 KB reserved

/**
 * Reset Vector
 *
 * Address where the CPU's Program Counter (PC) is initialized on reset.
 *
 * On power-on or system reset:
 * - PC = ResetVector (0x0)
 * - SP = RamEnd (stack grows downward from end of RAM)
 *
 * The bootloader or program binary is loaded at this address, so the
 * CPU starts executing from the first instruction immediately after reset.
 */
constexpr Address ResetVector = 0x00000000;

/**
 * Stack Initialization
 *
 * The stack pointer (SP) is initialized to point to the end of RAM.
 * The stack grows downward (toward lower addresses) as functions are called.
 *
 * This gives us the maximum possible stack space while keeping it separate
 * from the program code and data loaded at the beginning of RAM.
 */
constexpr Address InitialStackPointer = RamEnd;

/**
 * @brief Checks if an address falls within RAM.
 *
 * @param addr Address to check
 * @return true if addr is in [RamBase, RamEnd]
 */
constexpr bool IsRamAddress(Address addr) {
  return addr >= RamBase && addr <= RamEnd;
}

/**
 * @brief Checks if an address falls within MMIO space.
 *
 * @param addr Address to check
 * @return true if addr >= MmioBase
 */
constexpr bool IsMmioAddress(Address addr) { return addr >= MmioBase; }

/**
 * @brief Checks if an address is valid (either RAM or MMIO).
 *
 * @param addr Address to check
 * @return true if address is mapped to a device
 *
 * NOTE (KleaSCM) This doesn't guarantee the address is *currently* backed
 * by a connected device, just that it's in a valid region. The Bus will
 * perform finer-grained device selection based on actual connected devices.
 */
constexpr bool IsValidAddress(Address addr) {
  return IsRamAddress(addr) || IsMmioAddress(addr);
}

} // namespace Aurelia::System
