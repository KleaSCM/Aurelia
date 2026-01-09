/**
 * Flash Translation Layer Definitions.
 *
 * Defines logical-to-physical mapping structures and block states.
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#pragma once

#include "Core/Types.hpp"

namespace Aurelia::Storage::FTL {

using Lba = std::uint32_t; // Logical Block Address
using Pba = std::uint32_t; // Physical Block Address

enum class BlockState : Core::Byte {
  Free,   // Erased and available for allocation
  Active, // Currently being written to
  Full,   // Completely written, valid data
  Bad     // Marked as unusable (wear or factory bad)
};

struct BlockInfo {
  BlockState State = BlockState::Free;
  std::uint32_t EraseCount = 0;
  std::uint64_t ValidPageBitmap = 0; // Bit 1 = Valid, 0 = Invalid/Free
};

// Magic number stored in OOB to identify valid logical blocks
constexpr Core::Word FtlMagic = 0xDEADBEEF;

struct OobMetadata {
  Core::Word Magic;
  Lba LogicalAddress;
};

} // namespace Aurelia::Storage::FTL
