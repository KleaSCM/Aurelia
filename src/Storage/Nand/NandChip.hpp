/**
 * NAND Chip Simulation.
 *
 * Manages the array of Blocks and enforces Program/Erase constraints.
 * Supports Data and OOB (Spare) area access.
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#pragma once

#include "Storage/Nand/NandDefs.hpp"
#include <span>
#include <vector>

namespace Aurelia::Storage::Nand {

class NandChip {
public:
  explicit NandChip(std::size_t numBlocks);

  // NOTE (KleaSCM) Reads a full page into the provided buffer.
  // Optional: Read OOB if oobBuffer is provided (not empty).
  [[nodiscard]] NandStatus ReadPage(std::size_t blockIdx, std::size_t pageIdx,
                                    std::span<Core::Byte> buffer,
                                    std::span<Core::Byte> oobBuffer = {});

  // NOTE (KleaSCM) Programs a page and optional OOB.
  // Enforces bitwise AND constraints on both Data and OOB.
  [[nodiscard]] NandStatus
  ProgramPage(std::size_t blockIdx, std::size_t pageIdx,
              std::span<const Core::Byte> data,
              std::span<const Core::Byte> oobData = {});

  [[nodiscard]] NandStatus EraseBlock(std::size_t blockIdx);

  [[nodiscard]] std::size_t GetBlockCount() const;

private:
  std::vector<Block> m_Blocks;
};

} // namespace Aurelia::Storage::Nand
