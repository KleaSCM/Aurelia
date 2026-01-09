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
                              std::span<Core::Byte> buffer) {
  if (blockIdx >= m_Blocks.size() || pageIdx >= PagesPerBlock) {
    return NandStatus::InvalidAddress;
  }

  if (buffer.size() < PageDataSize) {
    return NandStatus::InvalidAddress; // Buffer too small
  }

  // NOTE (KleaSCM) Simple copy. In real HW, this would take time.
  const auto &page = m_Blocks[blockIdx].Pages[pageIdx];
  std::copy(page.Data.begin(), page.Data.end(), buffer.begin());

  return NandStatus::Success;
}

NandStatus NandChip::ProgramPage(std::size_t blockIdx, std::size_t pageIdx,
                                 std::span<const Core::Byte> data) {
  if (blockIdx >= m_Blocks.size() || pageIdx >= PagesPerBlock) {
    return NandStatus::InvalidAddress;
  }

  auto &page = m_Blocks[blockIdx].Pages[pageIdx];

  // NOTE (KleaSCM) Physics Verification:
  // Check that we are NOT trying to flip any 0 -> 1.
  // Existing (0) & Incoming (1) == 0 ? No.
  // Logic: If (Existing & Incoming) != Incoming, then we have a mismatch where
  // Existing was 0. Actually simpler: We want New = Old & Incoming. If (Old &
  // Incoming) != Incoming, it means Incoming has a 1 where Old has a 0.

  for (std::size_t i = 0; i < PageDataSize && i < data.size(); ++i) {
    Core::Byte oldByte = page.Data[i];
    Core::Byte newByte = data[i];

    // NOTE (KleaSCM) If we try to write a '1' (newByte bit set) but the old bit
    // is '0', that's physically impossible without an erase.
    if ((oldByte & newByte) != newByte) {
      return NandStatus::WriteError;
    }
  }

  // NOTE (KleaSCM) Perform the Programming (AND operation).
  // use bitwise AND to simulate the accumulation of electrons (0).
  for (std::size_t i = 0; i < PageDataSize && i < data.size(); ++i) {
    page.Data[i] &= data[i];
  }

  return NandStatus::Success;
}

NandStatus NandChip::EraseBlock(std::size_t blockIdx) {
  if (blockIdx >= m_Blocks.size()) {
    return NandStatus::InvalidAddress;
  }

  // NOTE (KleaSCM) Reset the entire block to 0xFF (Initialized state).
  m_Blocks[blockIdx].Erase();

  return NandStatus::Success;
}

} // namespace Aurelia::Storage::Nand
