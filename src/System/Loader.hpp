/**
 * Aurelia System Binary Loader.
 *
 * Loads assembled binary programs into system RAM via the Bus.
 * This is the bridge between the assembler output (.bin files) and
 * the running CPU simulation.
 *
 * USAGE WORKFLOW:
 * 1. Assemble program: `asm program.s -o program.bin`
 * 2. Create loader: `Loader loader(bus);`
 * 3. Load binary: `loader.LoadBinary("program.bin", 0x0);`
 * 4. Reset CPU and run
 *
 * LOADING STRATEGY:
 * The loader reads the binary file and uses the Bus to sequentially
 * write bytes to RAM. This simulates DMA or a bootloader copying
 * program code from external storage into main memory.
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#pragma once

#include "Bus/Bus.hpp"
#include "Core/Types.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace Aurelia::System {

// Import Address type for convenience
using Address = Core::Address;

class Loader {
public:
  /**
   * @brief Constructs a Loader with access to the system Bus.
   *
   * @param bus Reference to the Bus used for writing to RAM
   *
   * NOTE (KleaSCM) The Bus must have RAM already connected at the
   * target load address before calling Load methods.
   */
  explicit Loader(Bus::Bus &bus);

  /**
   * @brief Loads a binary file into RAM at the specified address.
   *
   * @param filename Path to binary file (e.g., "program.bin")
   * @param loadAddress Target address in RAM (default: 0x0)
   * @return true if file loaded successfully, false on error
   *
   * OPERATION:
   * 1. Read entire binary file into memory buffer
   * 2. Write buffer contents to Bus sequentially
   * 3. Bus routes writes to RAM device
   *
   * ERROR CONDITIONS:
   * - File not found or cannot be opened
   * - Load address + file size exceeds RAM bounds
   * - Bus write failure (device not connected, etc.)
   *
   * EXAMPLE:
   *   Loader loader(bus);
   *   if (!loader.LoadBinary("boot.bin", 0x0)) {
   *       std::cerr << "Failed to load bootloader\n";
   *   }
   */
  [[nodiscard]] bool LoadBinary(const std::string &filename,
                                Address loadAddress = 0x0);

  /**
   * @brief Loads raw binary data into RAM at the specified address.
   *
   * @param data Binary data to load
   * @param loadAddress Target address in RAM (default: 0x0)
   * @return true if data loaded successfully, false on error
   *
   * USE CASE:
   * Useful for tests where you want to load hand-crafted byte sequences
   * without creating temporary files:
   *
   *   std::vector<uint8_t> program = {0x00, 0x00, 0x00, 0x80}; // MOV R0, #0
   *   loader.LoadData(program, 0x0);
   */
  [[nodiscard]] bool LoadData(const std::vector<std::uint8_t> &data,
                              Address loadAddress = 0x0);

  /**
   * @brief Returns the last error message.
   *
   * @return Error description (empty if no error)
   *
   * USAGE:
   *   if (!loader.LoadBinary("test.bin")) {
   *       std::cerr << "Load failed: " << loader.GetErrorMessage() << "\n";
   *   }
   */
  [[nodiscard]] std::string GetErrorMessage() const { return m_ErrorMessage; }

private:
  Bus::Bus &m_Bus;
  std::string m_ErrorMessage;

  /**
   * @brief Internal helper to write data to Bus.
   *
   * Writes byte sequence to Bus starting at loadAddress.
   * Updates m_ErrorMessage on failure.
   *
   * @param data Bytes to write
   * @param loadAddress Starting address
   * @return true if all writes succeeded
   */
  bool WriteToBus(const std::vector<std::uint8_t> &data, Address loadAddress);
};

} // namespace Aurelia::System
