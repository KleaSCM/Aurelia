/**
 * NAND Flash Definitions.
 *
 * Defines the physical structure of a NAND component: Pages, OOB, and Blocks.
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#pragma once

#include "Core/Types.hpp"
#include <array>

namespace Aurelia::Storage::Nand {

// NOTE (KleaSCM) Standard 4KB Page Size with 64 bytes OOB (Out-Of-Band) area
// for ECC/MetaData.
constexpr std::size_t PageDataSize = 4096;
constexpr std::size_t OobSize = 64;

// NOTE (KleaSCM) A Block consists of 64 Pages. This is the smallest Erasable
// Unit.
constexpr std::size_t PagesPerBlock = 64;

struct Page {
  std::array<Core::Byte, PageDataSize> Data;
  std::array<Core::Byte, OobSize> Oob;

  // NOTE (KleaSCM) Constructor initializes memory to 0xFF (Erased State).
  // In NAND flash, an "empty" cell reads as 1, not 0.
  Page() {
    Data.fill(0xFF);
    Oob.fill(0xFF);
  }
};

struct Block {
  std::array<Page, PagesPerBlock> Pages;
  bool IsBad = false;           // Factory bad block marker
  std::uint32_t EraseCount = 0; // For wear leveling simulation

  // NOTE (KleaSCM) Erasing a block resets all bits to 1 (0xFF).
  void Erase() {
    for (auto &page : Pages) {
      page.Data.fill(0xFF);
      page.Oob.fill(0xFF);
    }
    EraseCount++;
  }
};

enum class NandStatus {
  Success,
  WriteError, // Tried to flip 0 -> 1 without erase
  InvalidAddress
};

} // namespace Aurelia::Storage::Nand
