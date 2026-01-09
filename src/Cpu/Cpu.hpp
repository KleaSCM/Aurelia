/**
 * Aurelia CPU Core.
 *
 * Implements the Central Processing Unit.
 * Connects to the System Bus as a Master device.
 * Maintains architectural state (Registers, PC, Flags).
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#pragma once

#include "Bus/Bus.hpp"
#include "Core/ITickable.hpp"
#include "Cpu/CpuDefs.hpp"
#include "Cpu/InstructionDefs.hpp"
#include <array>

namespace Aurelia::Cpu {

// Pipeline Stages
enum class CpuState { Fetch, Decode, Execute, Memory, WriteBack };

class Cpu : public Core::ITickable {
public:
  Cpu();

  void ConnectBus(Bus::Bus *bus);
  void Reset(Core::Address startAddress);

  void OnTick() override;

  // Debug / Inspection API
  [[nodiscard]] Core::Word GetRegister(Register reg) const;
  void SetRegister(Register reg, Core::Word value);

  [[nodiscard]] Core::Address GetPC() const;
  void SetPC(Core::Address value);

  [[nodiscard]] const Flags &GetFlags() const;
  [[nodiscard]] CpuState GetState() const { return m_State; }
  [[nodiscard]] bool IsHalted() const { return m_Halted; }

private:
  Bus::Bus *m_Bus = nullptr;

  // Architectural State
  std::array<Core::Word, static_cast<std::size_t>(Register::Count)> m_GPR;
  Core::Address m_PC = 0;
  Flags m_Flags;

  // Pipeline State
  CpuState m_State = CpuState::Fetch;

  // Pipeline Latches
  Instruction m_CurrentInstr; // Fetch -> Decode
  Core::Word m_OpA = 0;       // Decode -> Execute
  Core::Word m_OpB = 0;       // Decode -> Execute
  Core::Word m_AluResult = 0; // Execute -> Memory/WB
  Core::Data m_MemData = 0;   // Memory -> WB

  bool m_Halted = false; // HALT instruction executed
  int m_MicroOp = 0;     // For multi-cycle stages (Fetch/Memory)
};

} // namespace Aurelia::Cpu
