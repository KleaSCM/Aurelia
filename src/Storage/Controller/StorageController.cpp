/**
 * Storage Controller Implementation (NVMe-like).
 *
 * Implements Doorbell-based command processing and DMA.
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#include "Storage/Controller/StorageController.hpp"
#include "Core/BitManip.hpp"
#include <cstring>

namespace Aurelia::Storage::Controller {

StorageController::StorageController(FTL::Ftl *ftl) : m_Ftl(ftl) {}

void StorageController::ConnectBus(Bus::Bus *bus) { m_Bus = bus; }

void StorageController::SetBaseAddress(Core::Address baseAddr) {
  m_BaseAddr = baseAddr;
}

bool StorageController::IsAddressInRange(Core::Address addr) const {
  // Simple 4KB register space
  return addr >= m_BaseAddr && addr < m_BaseAddr + 0x2000;
}

bool StorageController::OnRead(Core::Address addr, Core::Data &outData) {
  Core::Address offset = addr - m_BaseAddr;
  outData = 0;

  if (offset == Regs::CSTS) {
    outData = static_cast<Core::Data>(m_CSTS);
  } else if (offset == Regs::CC) {
    outData = static_cast<Core::Data>(m_CC);
  } else if (offset == Regs::VS) {
    outData = 0x00010000; // 1.0.0
  }
  // Doorbells are usually Write-Only or Read-as-Zero
  return true;
}

bool StorageController::OnWrite(Core::Address addr, Core::Data inData) {
  Core::Address offset = addr - m_BaseAddr;

  if (offset == Regs::CC) {
    m_CC = inData;
    // Enable bit (Bit 0) transition
    if (Core::CheckBit(m_CC, 0)) {
      // Ready transition
      m_CSTS |= static_cast<Core::Word>(ControllerStatus::Ready);
    } else {
      // Reset
      m_CSTS &= ~static_cast<Core::Word>(ControllerStatus::Ready);
      // Reset State
      m_SQ0Head = m_SQ0TDBL = 0;
      m_CQ0Tail = m_CQ0HDBL = 0;
    }
  } else if (offset == Regs::ASQ_LO) {
    m_ASQ = inData;
  } else if (offset == Regs::ACQ_LO) {
    m_ACQ = inData;
  } else if (offset == Regs::SQ0TDBL) {
    // Host writes new Tail
    m_SQ0TDBL = static_cast<std::uint16_t>(inData & 0xFFFF);
    // Check if new commands available
    if (m_SQ0TDBL != m_SQ0Head) {
      FetchCommand();
    }
  } else if (offset == Regs::CQ0HDBL) {
    // Host acknowledges completions
    m_CQ0HDBL = static_cast<std::uint16_t>(inData & 0xFFFF);
  }

  return true;
}

void StorageController::OnTick() {
  if (m_BusyTicks > 0) {
    m_BusyTicks--;
    if (m_BusyTicks == 0 && m_HasPendingCmd) {
      ExecuteCommand();
    }
  }
}

void StorageController::FetchCommand() {
  if (!m_Bus || m_BusyTicks > 0)
    return;

  // DMA Read from ASQ (SQ Entry size = 64 bytes)
  Core::Address cmdAddr = m_ASQ + (m_SQ0Head * 64);

  Core::Data w0, w10, w12, prp1;
  // Dword 0: Opcode (Bits 0-7)
  m_Bus->Read(cmdAddr, w0);
  m_PendingCmd.Opcode = static_cast<Core::Byte>(w0 & 0xFF);

  // Dword 6-7: PRP1
  m_Bus->Read(cmdAddr + 24, prp1);
  m_PendingCmd.Prp1 = prp1;

  // Dword 10: LBA Lo
  m_Bus->Read(cmdAddr + 40, w10);
  m_PendingCmd.Dword10 = static_cast<std::uint32_t>(w10);

  // Dword 12: Length
  m_Bus->Read(cmdAddr + 48, w12);
  m_PendingCmd.Dword12 = static_cast<std::uint32_t>(w12);

  m_SQ0Head++; // Advance Head
  m_HasPendingCmd = true;
  m_BusyTicks = 5; // Simulate Access Time
}

void StorageController::ExecuteCommand() {
  m_HasPendingCmd = false;
  std::uint16_t status = 0; // Success

  if (m_PendingCmd.Opcode == static_cast<Core::Byte>(NvmeOpcode::Write)) {
    // DMA Read Data from PRP1 -> FTL Write
    // Block Size = 4KB. Length = Dword12 + 1 (0-based)
    // For now, support 1 block transfer
    std::vector<Core::Byte> buffer(4096);
    // DMA Read (Byte by Byte... slow but strict)
    for (size_t i = 0; i < 4096; ++i) {
      Core::Data b;
      // Bus 8-bit read not implemented, assume 32-bit aligned buffer
      // Just Read word every 4 bytes
      if (i % 4 == 0) {
        m_Bus->Read(m_PendingCmd.Prp1 + i, b);
        std::memcpy(&buffer[i], &b, 4);
      }
    }

    auto ftlStatus = m_Ftl->Write(m_PendingCmd.Dword10, buffer);
    if (ftlStatus != Nand::NandStatus::Success)
      status = 0x0001; // Internal Error

  } else if (m_PendingCmd.Opcode == static_cast<Core::Byte>(NvmeOpcode::Read)) {
    std::vector<Core::Byte> buffer(4096);
    auto ftlStatus = m_Ftl->Read(m_PendingCmd.Dword10, buffer);
    if (ftlStatus != Nand::NandStatus::Success)
      status = 0x0281; // Unrecovered Read Error

    // DMA Write Data -> PRP1
    for (size_t i = 0; i < 4096; i += 4) {
      Core::Data b;
      std::memcpy(&b, &buffer[i], 4);
      m_Bus->Write(m_PendingCmd.Prp1 + i, b);
    }
  } else {
    status = 0x0001; // Invalid Command
  }

  PostCompletion(0, status >> 1); // Phase bit handling omitted for brevity
}

void StorageController::PostCompletion(std::uint16_t /*cid*/,
                                       std::uint16_t status) {
  // Write CQE to ACQ
  Core::Address cqeAddr = m_ACQ + (m_CQ0Tail * 16);

  // Write Status (Dword 3)
  // Status | Phase Bit (Flip Phase every pass... simplify to 1)
  Core::Data statusWord = (static_cast<Core::Data>(status) << 17) | 1;
  m_Bus->Write(cqeAddr + 12, statusWord);

  m_CQ0Tail++;
  // Interrupt Logic Here
}

} // namespace Aurelia::Storage::Controller
