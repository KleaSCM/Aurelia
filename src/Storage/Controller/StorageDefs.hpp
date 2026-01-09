/**
 * MMIO Register Map (NVMe Compliant Subset).
 *
 * Defines the Controller Capabilities, Status, and Queue Doorbell registers.
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#pragma once

#include "Core/Types.hpp"

namespace Aurelia::Storage::Controller {

namespace Regs {
// Controller Capabilities (Read-Only)
constexpr Core::Address CAP_LO = 0x00;
constexpr Core::Address CAP_HI = 0x04;

// Version
constexpr Core::Address VS = 0x08;

// Interrupt Mask Set/Clear
constexpr Core::Address INTMS = 0x0C;
constexpr Core::Address INTMC = 0x10;

// Controller Configuration
constexpr Core::Address CC = 0x14;

// Controller Status
constexpr Core::Address CSTS = 0x1C;

// Admin Queue Attributes
constexpr Core::Address AQA = 0x24;

// Admin Submission Queue Base Address
constexpr Core::Address ASQ_LO = 0x28;
constexpr Core::Address ASQ_HI = 0x2C;

// Admin Completion Queue Base Address
constexpr Core::Address ACQ_LO = 0x30;
constexpr Core::Address ACQ_HI = 0x34;

// Doorbell Registers (Stride = 4 bytes)
// SQ0TDBL (Submission Queue 0 Tail Doorbell)
constexpr Core::Address SQ0TDBL = 0x1000;
// CQ0HDBL (Completion Queue 0 Head Doorbell)
constexpr Core::Address CQ0HDBL = 0x1004;

} // namespace Regs

enum class ControllerStatus : Core::Word {
  Ready = 1 << 0,
  CFS = 1 << 1, // Controller Fatal Status
};

enum class ErrorCode : Core::Data { None = 0, BufferOverflow = 1 };

// Submission Queue Entry (Simplified 64-byte command)
struct SubmissionQueueEntry {
  Core::Byte Opcode;
  Core::Byte Flags;
  Core::Word Reserved;
  Core::Address Prp1; // Data Buffer Address
  Core::Address Prp2;
  std::uint32_t Dword10; // LBA (Lower 32 bits)
  std::uint32_t Dword11; // LBA (Upper 32 bits) - Simplified
  std::uint32_t Dword12; // Length in Blocks
};

// Completion Queue Entry (16 bytes)
struct CompletionQueueEntry {
  std::uint32_t Dword0; // Command Specific
  std::uint32_t Reserved;
  std::uint16_t SqHeadPointer;
  std::uint16_t SqId;
  std::uint16_t CommandId;
  std::uint16_t Status; // Phase Tag (P) | Status Field (SCT/SC)
};

enum class NvmeOpcode : Core::Byte { Write = 0x01, Read = 0x02 };

} // namespace Aurelia::Storage::Controller
