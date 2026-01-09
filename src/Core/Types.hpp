/**
 * Core Types Definition.
 *
 * Defines the fundamental primitive types used throughout the Aurelia
 * architecture. Enforces strict 64-bit architecture constraints.
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#pragma once

#include <cstdint>

namespace Aurelia::Core {

// -------------------------------------------------------------------------
// Primitive Types
// -------------------------------------------------------------------------

// Represents a system memory address.
// Aurelia uses a flat 64-bit physical address space.
using Address = std::uint64_t;

// Represents a unit of data transferred across the bus (Word).
// Matching the architecture width of 64 bits.
using Data = std::uint64_t;

// Represents a standard 8-bit byte.
// Used for byte-addressable operations and storage.
using Byte = std::uint8_t;

// Represents a machine word (alias for Data for semantic clarity).
using Word = std::uint64_t;

// Represents a count of system clock cycles.
using TickCount = std::uint64_t;

// -------------------------------------------------------------------------
// Constants
// -------------------------------------------------------------------------

constexpr Data WordSize = sizeof(Word);
constexpr Data WordBits = WordSize * 8;

} // namespace Aurelia::Core
