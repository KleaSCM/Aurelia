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
   * Use Bus::Write() for direct DMA-style writes.
   * This bypasses the transactional timing and writes immediately.
   */
  for (std::size_t i = 0; i < data.size(); ++i) {
    Address addr = loadAddress + i;
    Core::Data byteData = static_cast<Core::Data>(data[i]);

    if (!m_Bus.Write(addr, byteData)) {
      std::ostringstream oss;
      oss << "Write failed at 0x" << std::hex << addr;
      m_ErrorMessage = oss.str();
      return false;
    }
  }

  m_ErrorMessage.clear();
  return true;
}

} // namespace Aurelia::System
