/**
 * Aurelia System Binary Loader Implementation.
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#include "System/Loader.hpp"
#include "System/MemoryMap.hpp"
#include <fstream>
#include <sstream>

namespace Aurelia::System {

Loader::Loader(Bus::Bus &bus) : m_Bus(bus) {}

bool Loader::LoadBinary(const std::string &filename, Address loadAddress) {
  /**
   * STAGE 1: READ FILE INTO BUFFER
   *
   * Open binary file and read entire contents into memory.
   * We use binary mode (std::ios::binary) to prevent any text
   * translation that might corrupt machine code.
   */
  std::ifstream file(filename, std::ios::in | std::ios::binary);
  if (!file.is_open()) {
    m_ErrorMessage = "Cannot open file: " + filename;
    return false;
  }

  // Read entire file into buffer using streambuf iterator
  std::vector<std::uint8_t> buffer((std::istreambuf_iterator<char>(file)),
                                   std::istreambuf_iterator<char>());

  file.close();

  if (buffer.empty()) {
    m_ErrorMessage = "File is empty: " + filename;
    return false;
  }

  /**
   * STAGE 2: VALIDATE LOAD ADDRESS
   *
   * Ensure the target address + file size doesn't exceed RAM bounds.
   * This prevents accidentally writing into MMIO space or unmapped regions.
   */
  Address endAddress = loadAddress + buffer.size() - 1;

  if (!IsRamAddress(loadAddress)) {
    std::ostringstream oss;
    oss << "Load address 0x" << std::hex << loadAddress
        << " is not in RAM region";
    m_ErrorMessage = oss.str();
    return false;
  }

  if (!IsRamAddress(endAddress)) {
    std::ostringstream oss;
    oss << "Program too large: ends at 0x" << std::hex << endAddress
        << " (exceeds RAM bounds)";
    m_ErrorMessage = oss.str();
    return false;
  }

  /**
   * STAGE 3: WRITE TO BUS
   *
   * Delegate actual Bus write to helper method.
   */
  return WriteToBus(buffer, loadAddress);
}

bool Loader::LoadData(const std::vector<std::uint8_t> &data,
                      Address loadAddress) {
  if (data.empty()) {
    m_ErrorMessage = "Cannot load empty data";
    return false;
  }

  // Validate address range
  Address endAddress = loadAddress + data.size() - 1;

  if (!IsRamAddress(loadAddress) || !IsRamAddress(endAddress)) {
    std::ostringstream oss;
    oss << "Data range [0x" << std::hex << loadAddress << ", 0x" << endAddress
        << "] exceeds RAM bounds";
    m_ErrorMessage = oss.str();
    return false;
  }

  return WriteToBus(data, loadAddress);
}

bool Loader::WriteToBus(const std::vector<std::uint8_t> &data,
                        Address loadAddress) {
  /**
   * BUS WRITE STRATEGY
   *
   * We write data to the Bus one byte at a time using 8-bit writes.
   * The Bus will:
   * 1. Decode the address
   * 2. Select the RAM device
   * 3. Route the write to RAM
   *
   * NOTE (KleaSCM) In a real system this would be a DMA transfer
   * or burst write for efficiency. Here we simulate byte-by-byte
   * writes for simplicity and to exercise the Bus interface.
   *
   * BYTE ORDER:
   * Data is written in the same order it appears in the file.
   * Assembler outputs little-endian, so multi-byte values will
   * have LSB first, MSB last.
   */
  for (std::size_t i = 0; i < data.size(); ++i) {
    Address addr = loadAddress + i;
    Core::Data byteData = static_cast<Core::Data>(data[i]);

    // Set address and data on Bus
    m_Bus.SetAddress(addr);
    m_Bus.SetData(byteData);

    // Initiate write transaction
    m_Bus.SetControl(Bus::ControlSignal::Write, true);
    m_Bus.OnTick(); // Process the write
    m_Bus.SetControl(Bus::ControlSignal::Write, false);

    /**
     * ERROR DETECTION
     *
     * If the Bus signals an error (e.g., no device at this address,
     * device reported failure), we abort the load immediately.
     *
     * TODO (Future): Add proper error signaling from Bus
     * For now, we assume all writes succeed if we're in RAM range.
     */
  }

  m_ErrorMessage.clear();
  return true;
}

} // namespace Aurelia::System
