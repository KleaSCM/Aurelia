/**
 * Storage Controller Integration Tests (NVMe).
 *
 * Verifies Doorbell, Submission Queue, and Completion Queue logic.
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#include "Bus/Bus.hpp"
#include "Memory/RamDevice.hpp"
#include "Storage/Controller/StorageController.hpp"
#include "Storage/FTL/Ftl.hpp"
#include "Storage/Nand/NandChip.hpp"
#include <catch2/catch_test_macros.hpp>

using namespace Aurelia::Storage::Controller;
using namespace Aurelia::Storage::FTL;
using namespace Aurelia::Storage::Nand;
using namespace Aurelia;

class NvmeControllerTest {
protected:
  NandChip nand;
  Ftl ftl;
  StorageController controller;
  Bus::Bus bus;
  Memory::RamDevice ram;

public:
  NvmeControllerTest()
      : nand(64), ftl(&nand, 64), controller(&ftl), ram(0x10000, 0) {
    ram.SetBaseAddress(0x00000000);        // RAM at 0
    controller.SetBaseAddress(0xF0000000); // Controller at F0000000

    bus.ConnectDevice(&ram);
    bus.ConnectDevice(&controller);

    controller.ConnectBus(&bus);

    // Initialize Controller
    bus.Write(0xF0000000 + Regs::CC, 1); // Enable
  }

  void WriteSQE(int index, NvmeOpcode op, std::uint32_t lba,
                Core::Address prp1) {
    SubmissionQueueEntry sqe{};
    sqe.Opcode = static_cast<Core::Byte>(op);
    sqe.Dword10 = lba;
    sqe.Prp1 = prp1;

    // Write 64 bytes to RAM (ASQ Base 0 assumed)
    Core::Address addr = static_cast<Core::Address>(index) * 64;

    // Simple copy to RAM via Bus DMA/Debug
    // We can't memcpy to RamDevice, use Bus::Write
    // Hacky write of struct
    bus.Write(addr + 0, sqe.Opcode);
    bus.Write(addr + 24, sqe.Prp1);
    bus.Write(addr + 40, sqe.Dword10);
  }

  void RingDoorbell(int tail) {
    bus.Write(0xF0000000 + Regs::SQ0TDBL, static_cast<Core::Data>(tail));
  }

  void WaitForCompletion(int) {
    // Poll CQ for completion status

    // Simulate Ticks
    // Increased tick count slightly to ensures completion in strict mode
    for (int i = 0; i < 50; ++i) {
      bus.OnTick();
      controller.OnTick();
    }
  }
};

TEST_CASE_METHOD(NvmeControllerTest, "NvmeController - WriteAndReadBasic") {
  // 1. Write Data to RAM for Write Command
  bus.Write(0x1000, 0xEFBEADDE); // Data at 0x1000 = 0xDEADBEEF

  // 2. Setup SQE 0: Write LBA 5, PRP1=0x1000
  WriteSQE(0, NvmeOpcode::Write, 5, 0x1000);

  // 3. Ring Doorbell (Tail=1)
  RingDoorbell(1);

  // 4. Wait
  WaitForCompletion(1);

  // 5. Setup SQE 1: Read LBA 5, PRP1=0x2000
  WriteSQE(1, NvmeOpcode::Read, 5, 0x2000);

  // 6. Ring Doorbell (Tail=2)
  RingDoorbell(2);

  // 7. Wait
  WaitForCompletion(2);

  // 8. Verify Data at 0x2000 matches 0x1000
  Core::Data val;
  bus.Read(0x2000, val);
  CHECK(val == 0xEFBEADDE);
}
