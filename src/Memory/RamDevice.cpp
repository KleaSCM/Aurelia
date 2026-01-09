/**
 * RAM Device Implementation.
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#include "Memory/RamDevice.hpp"
#include <cstring> // for memcpy

namespace Aurelia::Memory {

RamDevice::RamDevice(std::size_t sizeBytes, Core::TickCount latency)
    : m_BaseAddr(0), m_Size(sizeBytes), m_Latency(latency),
      m_CurrentWaitTicks(0) {
  m_Storage.resize(sizeBytes, 0);
}

void RamDevice::SetBaseAddress(Core::Address baseAddr) {
  m_BaseAddr = baseAddr;
}

bool RamDevice::IsAddressInRange(Core::Address addr) const {
  return addr >= m_BaseAddr && addr < (m_BaseAddr + m_Size);
}

void RamDevice::OnTick() {
  if (m_CurrentWaitTicks > 0) {
    m_CurrentWaitTicks--;
  }
}

bool RamDevice::OnRead(Core::Address addr, Core::Data &outData) {
  if (m_Latency == 0) {
    // Zero latency path
  } else if (m_CurrentWaitTicks > 0) {
    return false; // Still waiting
  } else if (!m_IsBusy) {
    // Start new wait
    m_CurrentWaitTicks = m_Latency;
    m_IsBusy = true;
    return false;
  }
  // Pass through if m_IsBusy is true AND ticks == 0 (Wait finished)
  m_IsBusy = false;

  // Perform Read
  // Calculate offset
  Core::Address offset = addr - m_BaseAddr;

  // Guard against out of bounds (should be caught by IsAddressInRange but
  // safety first)
  if (offset + sizeof(Core::Data) > m_Storage.size()) {
    outData = 0; // Bus Error?
    return true;
  }

  std::memcpy(&outData, &m_Storage[offset], sizeof(Core::Data));
  return true;
}

bool RamDevice::OnWrite(Core::Address addr, Core::Data inData) {
  if (m_Latency == 0) {
    // Zero latency path
  } else if (m_CurrentWaitTicks > 0) {
    return false; // Still waiting
  } else if (!m_IsBusy) {
    // Start new wait
    m_CurrentWaitTicks = m_Latency;
    m_IsBusy = true;
    return false;
  }
  // Pass through if m_IsBusy is true AND ticks == 0 (Wait finished)
  m_IsBusy = false;

  // Perform Write
  Core::Address offset = addr - m_BaseAddr;

  if (offset + sizeof(Core::Data) > m_Storage.size()) {
    return true;
  }

  std::memcpy(&m_Storage[offset], &inData, sizeof(Core::Data));
  return true;
}

} // namespace Aurelia::Memory
