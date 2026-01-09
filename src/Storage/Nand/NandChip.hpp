/**
 * NAND Chip Simulation.
 *
 * Manages the array of Blocks and enforces Program/Erase constraints.
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
  // NOTE (KleaSCM) Initialize with a fixed number of blocks.
  // Example: 1024 Blocks * 64 Pages * 4KB = ~256MB
  explicit NandChip(std::size_t numBlocks);

  // NOTE (KleaSCM) Reads a full page into the provided buffer.
  // Buffer must be at least PageDataSize bytes.
  [[nodiscard]] NandStatus ReadPage(std::size_t blockIdx, std::size_t pageIdx,
                                    std::span<Core::Byte> buffer);

  // NOTE (KleaSCM) Programs a page. Matches physical NAND behavior:
  // Can only transition bits 1->0. If existing bit is 0 and new bit is 1,
  // operation fails.
  [[nodiscard]] NandStatus ProgramPage(std::size_t blockIdx,
                                       std::size_t pageIdx,
                                       std::span<const Core::Byte> data);

  // NOTE (KleaSCM) Erases an entire block, resetting all bits to 1.
  [[nodiscard]] NandStatus EraseBlock(std::size_t blockIdx);

private:
  std::vector<Block> m_Blocks;
};

} // namespace Aurelia::Storage::Nand
