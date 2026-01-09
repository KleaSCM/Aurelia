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

#include <array>

#include "Bus/Bus.hpp"
#include "Core/ITickable.hpp"
#include "Cpu/CpuDefs.hpp"
#include "Cpu/InstructionDefs.hpp"

namespace Aurelia::Cpu {

// Pipeline Stages
enum class CpuState { Fetch, Decode, Execute, Memory, WriteBack };

class Cpu : public Core::ITickable {
public:
  Cpu();

  void ConnectBus(Bus::Bus *Bus);
  void Reset(Core::Address StartAddress);

  void OnTick() override;

  // Debug / Inspection API
  [[nodiscard]] Core::Word GetRegister(Register Reg) const;
  void SetRegister(Register Reg, Core::Word Value);

  [[nodiscard]] Core::Address GetPC() const;
  void SetPC(Core::Address Value);

  [[nodiscard]] const Flags &GetFlags() const;
  [[nodiscard]] CpuState GetState() const { return State; }
  [[nodiscard]] bool IsHalted() const { return Halted; }

private:
  Bus::Bus *SystemBus = nullptr;

  // Architectural State
  std::array<Core::Word, static_cast<std::size_t>(Register::Count)> GPR;
  Core::Address PC = 0;
  Flags CurrentFlags;

  // Pipeline State
  CpuState State = CpuState::Fetch;

  // Pipeline Latches
  Instruction CurrentInstr; // Fetch -> Decode
  Core::Word OpA = 0;       // Decode -> Execute
  Core::Word OpB = 0;       // Decode -> Execute
  Core::Word AluResult = 0; // Execute -> Memory/WB
  Core::Data MemData = 0;   // Memory -> WB

  bool Halted = false; // HALT instruction executed
  int MicroOp = 0;     // For multi-cycle stages (Fetch/Memory)
};

} // namespace Aurelia::Cpu
