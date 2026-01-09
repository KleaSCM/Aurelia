/**
 * Flash Translation Layer Implementation.
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#include "Storage/FTL/Ftl.hpp"
#include <algorithm>
#include <bit>
#include <cstring>
#include <limits>

namespace Aurelia::Storage::FTL {

Ftl::Ftl(Nand::NandChip *nand, std::size_t totalBlocks)
    : m_Nand(nand), m_TotalBlocks(totalBlocks) {
  m_BlockTable.resize(m_TotalBlocks);
  ScanAndMount();

  if (m_FreeList.empty()) {
    // In a real scenario, this would trigger formatting or read-only mode.
    // Current simulation assumes fresh start capability via test setup.
  } else {
    AllocateNewActiveBlock();
  }
}

void Ftl::ScanAndMount() {
  std::vector<Core::Byte> buffer(Nand::PageDataSize);
  std::vector<Core::Byte> oob(Nand::OobSize);

  m_FreeList.clear();
  m_MappingTable.clear();

  // Scan all blocks to reconstruct mapping table from OOB metadata.
  // Reverse scan ensures FreeList pop order aligns with sequential allocation
  // for deterministic testing.
  for (std::size_t i = m_TotalBlocks; i > 0; --i) {
    std::size_t b = i - 1;
    bool blockIsEmpty = true;
    bool blockIsActive = false;

    // Check Page 0 first to determine if block is used
    if (m_Nand->ReadPage(b, 0, buffer, oob) != Nand::NandStatus::Success) {
      m_BlockTable[b].State = BlockState::Bad;
      continue;
    }

    OobMetadata meta;
    std::memcpy(&meta, oob.data(), sizeof(OobMetadata));

    if (meta.Magic == FtlMagic) {
      blockIsEmpty = false;
      m_MappingTable[meta.LogicalAddress] = static_cast<Pba>((b * 64) + 0);

      // Scan remaining pages to rebuild full map and find active frontier
      for (std::size_t p = 1; p < 64; ++p) {
        if (m_Nand->ReadPage(b, p, buffer, oob) != Nand::NandStatus::Success) {
          break; // Stop scanning if read fails (shouldn't happen on valid
                 // block)
        }
        std::memcpy(&meta, oob.data(), sizeof(OobMetadata));

        if (meta.Magic == FtlMagic) {
          m_MappingTable[meta.LogicalAddress] = static_cast<Pba>((b * 64) + p);
        } else {
          // Found empty page in used block -> This is the Active Block
          m_CurrentActiveBlock = b;
          m_CurrentPageOffset = p;
          m_BlockTable[b].State = BlockState::Active;
          blockIsActive = true;
          break; // Stop scanning this block
        }
      }

      if (!blockIsActive) {
        m_BlockTable[b].State = BlockState::Full;
      }
    }

    if (blockIsEmpty) {
      m_BlockTable[b].State = BlockState::Free;
      m_BlockTable[b].EraseCount = 0;
      m_FreeList.push_back(b);
    }
  }
}

std::size_t Ftl::AllocateNewActiveBlock() {
  if (m_FreeList.empty()) {
    if (!GarbageCollect()) {
      return std::numeric_limits<std::size_t>::max();
    }
    // Check again! GC might have succeeded but consumed the block immediately
    // (CopyBack)
    if (m_FreeList.empty()) {
      return std::numeric_limits<std::size_t>::max();
    }
  }

  std::size_t newBlock = m_FreeList.back();
  m_FreeList.pop_back();

  m_BlockTable[newBlock].State = BlockState::Active;
  m_BlockTable[newBlock].ValidPageBitmap =
      0; // New active block has no valid pages yet
  m_CurrentActiveBlock = newBlock;
  m_CurrentPageOffset = 0;

  return newBlock;
}

Nand::NandStatus Ftl::Write(Lba lba, std::span<const Core::Byte> data) {
  if (data.size() != Nand::PageDataSize) {
    return Nand::NandStatus::WriteError;
  }

  // Prepare OOB Metadata
  OobMetadata meta;
  meta.Magic = FtlMagic;
  meta.LogicalAddress = lba;

  std::vector<Core::Byte> oob(Nand::OobSize);
  std::memcpy(oob.data(), &meta, sizeof(OobMetadata));

  // Check L2P Table for existing mapping (Overwriting LBA)
  if (m_MappingTable.contains(lba)) {
    Pba oldPba = m_MappingTable[lba];
    std::size_t oldBlock = oldPba / 64;
    std::size_t oldPage = oldPba % 64;

    // Invalidate the old page in the Block Info Bitmap
    // Clear bit corresponding to oldPage.
    // Hardware would write a "0" to an OOB flag, here we track in RAM for model
    // accuracy.
    m_BlockTable[oldBlock].ValidPageBitmap &= ~(1ULL << oldPage);
  }

  // Allocate active block if none or full
  if (m_CurrentActiveBlock == std::numeric_limits<std::size_t>::max()) {
    m_CurrentActiveBlock = AllocateNewActiveBlock();
    if (m_CurrentActiveBlock == std::numeric_limits<std::size_t>::max()) {
      return Nand::NandStatus::WriteError; // No free blocks (GC required)
    }
    m_CurrentPageOffset = 0;
  }

  // Perform NAND Program
  Nand::NandStatus status =
      m_Nand->ProgramPage(m_CurrentActiveBlock, m_CurrentPageOffset, data, oob);

  if (status == Nand::NandStatus::Success) {
    // Update Mapping Table
    m_MappingTable[lba] =
        static_cast<Pba>((m_CurrentActiveBlock * 64) + m_CurrentPageOffset);

    // Mark new page as Valid
    m_BlockTable[m_CurrentActiveBlock].ValidPageBitmap |=
        (1ULL << m_CurrentPageOffset);

    m_CurrentPageOffset++;
    if (m_CurrentPageOffset >= 64) {
      m_BlockTable[m_CurrentActiveBlock].State = BlockState::Full;
      // Close the active block. Next write triggers new allocation.
      m_CurrentActiveBlock = std::numeric_limits<std::size_t>::max();
    }
  }

  return status;
}

Nand::NandStatus Ftl::Read(Lba lba, std::span<Core::Byte> buffer) {
  if (!m_MappingTable.contains(lba)) {
    // Unmapped LBA - We fill with 0xFF (erased state simulation)
    std::fill(buffer.begin(), buffer.end(), 0xFF);
    return Nand::NandStatus::Success; // Not strictly an error
  }

  Pba pba = m_MappingTable[lba];
  std::size_t block = pba / 64;
  std::size_t page = pba % 64;

  return m_Nand->ReadPage(block, page, buffer);
}

bool Ftl::GarbageCollect() {
  // 1. Greedy Victim Selection
  // Find the block with the MAXIMUM number of INVALID pages (Least Valid).
  std::size_t victimBlock = std::numeric_limits<std::size_t>::max();
  std::size_t minValidPages = 65; // Max is 64

  for (std::size_t i = 0; i < m_TotalBlocks; ++i) {
    if (i == m_CurrentActiveBlock)
      continue;

    const auto &info = m_BlockTable[i];
    // Only consider blocks that have some data (Active or Full)
    if (info.State == BlockState::Free || info.State == BlockState::Bad)
      continue;

    // Count set bits in ValidPageBitmap
    std::size_t validCount =
        static_cast<std::size_t>(std::popcount(info.ValidPageBitmap));

    if (validCount < minValidPages) {
      minValidPages = validCount;
      victimBlock = i;
    }
  }

  if (victimBlock == std::numeric_limits<std::size_t>::max()) {
    // No suitable victim found.
    return false;
  }

  // 2. Valid Page Migration (Copy-Back)
  // Read valid pages to RAM.
  struct PageBuffer {
    std::vector<Core::Byte> Data;
    Lba LogicalAddr;
  };
  std::vector<PageBuffer> validPages;

  std::vector<Core::Byte> buffer(Nand::PageDataSize);
  std::vector<Core::Byte> oob(Nand::OobSize);

  for (std::size_t p = 0; p < 64; ++p) {
    if (m_BlockTable[victimBlock].ValidPageBitmap & (1ULL << p)) {
      if (m_Nand->ReadPage(victimBlock, p, buffer, oob) ==
          Nand::NandStatus::Success) {
        OobMetadata meta;
        std::memcpy(&meta, oob.data(), sizeof(OobMetadata));

        // Sanity Check: Is this page actually mapped?
        if (m_MappingTable.contains(meta.LogicalAddress)) {
          Pba expectedPba = static_cast<Pba>((victimBlock * 64) + p);
          if (m_MappingTable[meta.LogicalAddress] == expectedPba) {
            validPages.push_back({buffer, meta.LogicalAddress});
          }
        }
      }
    }
  }

  // 3. Erase Victim
  if (m_Nand->EraseBlock(victimBlock) != Nand::NandStatus::Success) {
    m_BlockTable[victimBlock].State = BlockState::Bad;
    return false;
  }

  m_BlockTable[victimBlock].State = BlockState::Free;
  m_BlockTable[victimBlock].ValidPageBitmap = 0;
  m_BlockTable[victimBlock].EraseCount++;

  // 4. Commit Victim to Free List
  m_FreeList.push_back(victimBlock);

  // 5. Write Back (Resurrect Valid Data)
  for (const auto &page : validPages) {
    if (Write(page.LogicalAddr, page.Data) != Nand::NandStatus::Success) {
      return false;
    }
  }

  return true;
}

} // namespace Aurelia::Storage::FTL
