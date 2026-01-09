/**
 * Flash Translation Layer.
 *
 * Manages Logical to Physical mapping, Block Allocation, and State Persistence.
 * Implements a log-structured block management scheme.
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#pragma once

#include "Storage/FTL/FtlDefs.hpp"
#include "Storage/Nand/NandChip.hpp"
#include <map>
#include <vector>

namespace Aurelia::Storage::FTL {

class Ftl {
public:
  Ftl(Nand::NandChip *nand, std::size_t totalBlocks);

  [[nodiscard]] Nand::NandStatus Write(Lba lba,
                                       std::span<const Core::Byte> data);
  [[nodiscard]] Nand::NandStatus Read(Lba lba, std::span<Core::Byte> buffer);

  // For Testing
  [[nodiscard]] BlockInfo GetBlockInfo(std::size_t blockIdx) const {
    return m_BlockTable[blockIdx];
  }

private:
  Nand::NandChip *m_Nand;
  std::size_t m_TotalBlocks;

  // Mapping Tables
  std::map<Lba, Pba> m_MappingTable;
  std::vector<BlockInfo> m_BlockTable;
  std::vector<std::size_t> m_FreeList;

  // Active Block State
  std::size_t m_CurrentActiveBlock = 0;
  std::size_t m_CurrentPageOffset = 0;

  void ScanAndMount();
  std::size_t AllocateNewActiveBlock();
  void MarkBlockFull(std::size_t blockIdx);
};

} // namespace Aurelia::Storage::FTL
