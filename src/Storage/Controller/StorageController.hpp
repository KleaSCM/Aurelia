/**
 * Storage Controller Device.
 *
 * Implements the IBusDevice interface to expose FTL operations via MMIO.
 * Has an internal 4KB Page Buffer for data transfer.
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#pragma once

#include "Bus/Bus.hpp"
#include "Bus/IBusDevice.hpp"
#include "Storage/Controller/StorageDefs.hpp"
#include "Storage/FTL/Ftl.hpp"

namespace Aurelia::Storage::Controller {

class StorageController : public Bus::IBusDevice {
public:
  // NOTE (KleaSCM) Connects to the FTL backend.
  StorageController(FTL::Ftl *ftl);
  void ConnectBus(Bus::Bus *bus);

  // IBusDevice Interface
  [[nodiscard]] bool IsAddressInRange(Core::Address addr) const override;
  bool OnRead(Core::Address addr, Core::Data &outData) override;
  bool OnWrite(Core::Address addr, Core::Data inData) override;
  void OnTick() override;

  void SetBaseAddress(Core::Address baseAddr);

private:
  FTL::Ftl *m_Ftl;
  Core::Address m_BaseAddr = 0;

  // DMA Access (Bus Master)
  Bus::Bus *m_Bus = nullptr;

  // Internal Registers
  Core::Word m_CSTS = static_cast<Core::Word>(ControllerStatus::Ready);
  Core::Word m_CC = 0;

  // Queue State (Admin Queue Only for simplicity)
  Core::Address m_ASQ = 0; // Admin Submission Queue Base
  Core::Address m_ACQ = 0; // Admin Completion Queue Base

  std::uint16_t m_SQ0TDBL = 0; // Submission Queue 0 Tail Doorbell (Host writes)
  std::uint16_t m_SQ0Head = 0; // Internal Head Pointer (Controller reads)
  std::uint16_t m_CQ0HDBL = 0; // Completion Queue 0 Head Doorbell (Host writes)
  std::uint16_t m_CQ0Tail = 0; // Internal Tail Pointer (Controller writes)

  // Processing State
  Core::TickCount m_BusyTicks = 0;
  SubmissionQueueEntry m_PendingCmd{};
  bool m_HasPendingCmd = false;

  void FetchCommand();
  void ExecuteCommand();
  void PostCompletion(std::uint16_t cid, std::uint16_t status);
};

} // namespace Aurelia::Storage::Controller
