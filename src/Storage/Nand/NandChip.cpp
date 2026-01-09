/**
 * NAND Chip Implementation.
 *
 * Enforces the bitwise physics of NAND memory.
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#include "Storage/Nand/NandChip.hpp"
#include <algorithm>
#include <cstring>

namespace Aurelia::Storage::Nand {

NandChip::NandChip(std::size_t numBlocks) { m_Blocks.resize(numBlocks); }

NandStatus NandChip::ReadPage(std::size_t blockIdx, std::size_t pageIdx,
                              std::span<Core::Byte> buffer,
                              std::span<Core::Byte> oobBuffer) {
  if (blockIdx >= m_Blocks.size() || pageIdx >= PagesPerBlock) {
    return NandStatus::InvalidAddress;
  }

  if (buffer.size() < PageDataSize) {
    return NandStatus::InvalidAddress; // Buffer too small
  }

  const auto &page = m_Blocks[blockIdx].Pages[pageIdx];
  std::copy(page.Data.begin(), page.Data.end(), buffer.begin());

  // NOTE (KleaSCM) Read OOB if requested
  if (!oobBuffer.empty()) {
    if (oobBuffer.size() < OobSize)
      return NandStatus::InvalidAddress;
    std::copy(page.Oob.begin(), page.Oob.end(), oobBuffer.begin());
  }

  return NandStatus::Success;
}

NandStatus NandChip::ProgramPage(std::size_t blockIdx, std::size_t pageIdx,
                                 std::span<const Core::Byte> data,
                                 std::span<const Core::Byte> oobData) {
  if (blockIdx >= m_Blocks.size() || pageIdx >= PagesPerBlock) {
    return NandStatus::InvalidAddress;
  }

  auto &page = m_Blocks[blockIdx].Pages[pageIdx];

  // NOTE (KleaSCM) Physics Verification (Data Area)
  for (std::size_t i = 0; i < PageDataSize && i < data.size(); ++i) {
    if ((page.Data[i] & data[i]) != data[i]) {
      return NandStatus::WriteError;
    }
  }

  // NOTE (KleaSCM) Physics Verification (OOB Area)
  if (!oobData.empty()) {
    for (std::size_t i = 0; i < OobSize && i < oobData.size(); ++i) {
      if ((page.Oob[i] & oobData[i]) != oobData[i]) {
        return NandStatus::WriteError;
      }
    }
  }

  // NOTE (KleaSCM) Program Data
  for (std::size_t i = 0; i < PageDataSize && i < data.size(); ++i) {
    page.Data[i] &= data[i];
  }

  // NOTE (KleaSCM) Program OOB
  if (!oobData.empty()) {
    for (std::size_t i = 0; i < OobSize && i < oobData.size(); ++i) {
      page.Oob[i] &= oobData[i];
    }
  }

  return NandStatus::Success;
}

NandStatus NandChip::EraseBlock(std::size_t blockIdx) {
  if (blockIdx >= m_Blocks.size()) {
    return NandStatus::InvalidAddress;
  }

  m_Blocks[blockIdx].Erase();
  return NandStatus::Success;
}

std::size_t NandChip::GetBlockCount() const { return m_Blocks.size(); }

} // namespace Aurelia::Storage::Nand
