/**
 * Bus System Tests.
 *
 * Verifies address decoding and signal propagation.
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#include "Bus/Bus.hpp"
#include "Core/BitManip.hpp"
#include <catch2/catch_test_macros.hpp>

using namespace Aurelia;
using namespace Aurelia::Core;

using Aurelia::Bus::ControlSignal;
using Aurelia::Bus::IBusDevice;
using SystemBus = Aurelia::Bus::Bus;

class MockMemory : public IBusDevice {
public:
  Address BaseAddr = 0;
  Address Size = 1024;
  Data LastWritten = 0;
  Data DataToRead = 0xCAFEBABE;

  bool IsAddressInRange(Address addr) const override {
    return addr >= BaseAddr && addr < (BaseAddr + Size);
  }

  bool OnRead(Address, Data &outData) override {
    outData = DataToRead;
    return true; // Immediate response
  }

  bool OnWrite(Address, Data inData) override {
    LastWritten = inData;
    return true;
  }

  void OnTick() override {}
};

class BusTest {
protected:
  std::unique_ptr<SystemBus> bus;
  std::unique_ptr<MockMemory> mem1;

public:
  BusTest() {
    bus = std::make_unique<SystemBus>();
    mem1 = std::make_unique<MockMemory>();
  }
};

TEST_CASE_METHOD(BusTest, "Bus - AddressDecoding") {
  mem1->BaseAddr = 0x1000;

  bus->ConnectDevice(mem1.get());

  // Test Write
  bus->SetAddress(0x1000);
  bus->SetData(0x12345678);
  bus->SetControl(ControlSignal::Write, true);

  bus->OnTick();

  CHECK(mem1->LastWritten == 0x12345678);
}

TEST_CASE_METHOD(BusTest, "Bus - ReadOperation") {
  mem1->BaseAddr = 0x2000;
  mem1->DataToRead = 0xDEADBEEF;

  bus->ConnectDevice(mem1.get());

  bus->SetAddress(0x2000);
  bus->SetControl(ControlSignal::Read, true);

  bus->OnTick();

  CHECK(bus->GetState().DataBus == 0xDEADBEEF);
}

TEST_CASE_METHOD(BusTest, "Bus - BusFault") {
  // Write to invalid address 0xDEADBEEF (No device mapped)
  // Expect Error signal.

  // Setup Write
  bus->SetAddress(0xDEADBEEF);
  bus->SetControl(ControlSignal::Write, true);

  bus->OnTick();

  // Verify Error Bit (Bit 5)
  auto state = bus->GetState();
  CHECK(CheckBit(state.Control, 5));
}
