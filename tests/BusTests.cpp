/**
 * Bus System Tests.
 *
 * Verifies address decoding and signal propagation.
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#include "Bus/Bus.hpp"
#include <gtest/gtest.h>

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

TEST(BusTest, AddressDecoding) {
  SystemBus bus;
  MockMemory mem1;
  mem1.BaseAddr = 0x1000;

  bus.ConnectDevice(&mem1);

  // Test Write
  bus.SetAddress(0x1000);
  bus.SetData(0x12345678);
  bus.SetControl(ControlSignal::Write, true);

  bus.OnTick();

  EXPECT_EQ(mem1.LastWritten, 0x12345678);
}

TEST(BusTest, ReadOperation) {
  SystemBus bus;
  MockMemory mem1;
  mem1.BaseAddr = 0x2000;
  mem1.DataToRead = 0xDEADBEEF;

  bus.ConnectDevice(&mem1);

  bus.SetAddress(0x2000);
  bus.SetControl(ControlSignal::Read, true);

  bus.OnTick();

  EXPECT_EQ(bus.GetState().DataBus, 0xDEADBEEF);
}
