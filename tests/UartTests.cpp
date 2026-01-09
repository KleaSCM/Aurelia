/**
 * UART Device Unit Tests.
 *
 * Verifies UART register behavior, IRQ generation, and data transmission.
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#include "Peripherals/UartDevice.hpp"
#include <catch2/catch_test_macros.hpp>
#include <iostream>
#include <sstream>

using namespace Aurelia;
using namespace Aurelia::Peripherals;

TEST_CASE("UART - Address Range Check") {
  UartDevice uart;

  // Valid addresses within UART range
  REQUIRE(uart.IsAddressInRange(0xE0001000)); // Base (DATA reg)
  REQUIRE(uart.IsAddressInRange(0xE0001004)); // STATUS reg
  REQUIRE(uart.IsAddressInRange(0xE0001008)); // CONTROL reg
  REQUIRE(uart.IsAddressInRange(0xE0001FFF)); // End of 4KB page

  // Invalid addresses
  REQUIRE_FALSE(uart.IsAddressInRange(0xE0000FFF)); // Before UART
  REQUIRE_FALSE(uart.IsAddressInRange(0xE0002000)); // After UART (PIC region)
  REQUIRE_FALSE(uart.IsAddressInRange(0x00000000)); // RAM
}

TEST_CASE("UART - Initial State") {
  UartDevice uart;
  Core::Data data;

  // TX should be ready on power-up
  REQUIRE(uart.OnRead(0xE0001004, data)); // Read STATUS
  REQUIRE((data & 0x1) == 0x1);           // TX_READY bit set

  // RX should not be available
  REQUIRE((data & 0x2) == 0x0); // RX_AVAIL bit clear

  // No pending IRQ
  REQUIRE_FALSE(uart.HasIrq());
}

TEST_CASE("UART - Transmit Data") {
  UartDevice uart;

  // Redirect cout to capture UART output
  std::ostringstream capturedOutput;
  std::streambuf *oldCoutBuf = std::cout.rdbuf(capturedOutput.rdbuf());

  // Write 'H' to DATA register
  REQUIRE(uart.OnWrite(0xE0001000, 0x48));

  // Restore cout
  std::cout.rdbuf(oldCoutBuf);

  // Verify character was transmitted
  REQUIRE(capturedOutput.str() == "H");
}

TEST_CASE("UART - Receive Data") {
  UartDevice uart;
  Core::Data data;

  // Initially RX buffer empty
  REQUIRE(uart.OnRead(0xE0001000, data)); // Read DATA
  REQUIRE(data == 0x00);                  // Returns 0 when empty

  // Simulate incoming data
  uart.SimulateReceive(0x41); // 'A'

  // STATUS should show RX_AVAIL
  REQUIRE(uart.OnRead(0xE0001004, data)); // Read STATUS
  REQUIRE((data & 0x2) == 0x2);           // RX_AVAIL set

  // Read received byte
  REQUIRE(uart.OnRead(0xE0001000, data)); // Read DATA
  REQUIRE(data == 0x41);                  // Got 'A'

  // STATUS should clear RX_AVAIL
  REQUIRE(uart.OnRead(0xE0001004, data)); // Read STATUS
  REQUIRE((data & 0x2) == 0x0);           // RX_AVAIL cleared
}

TEST_CASE("UART - RX IRQ Generation") {
  UartDevice uart;
  Core::Data data;

  // Enable RX IRQ in CONTROL register
  REQUIRE(uart.OnWrite(0xE0001008, 0x08)); // Set RX_IRQ_EN (bit 3)

  // No IRQ yet (buffer empty)
  REQUIRE_FALSE(uart.HasIrq());

  // Simulate incoming data
  uart.SimulateReceive(0x42);

  // IRQ should now be pending
  REQUIRE(uart.HasIrq());

  // Clear IRQ
  uart.ClearIrq();
  REQUIRE_FALSE(uart.HasIrq());

  // But condition still exists (data in buffer)
  // Re-evaluate should re-assert IRQ
  uart.OnRead(0xE0001008, data); // Dummy read to trigger update
  // NOTE: IRQ stays cleared until UpdateIrqState called again
}

TEST_CASE("UART - TX IRQ Generation") {
  UartDevice uart;

  // Enable TX IRQ in CONTROL register
  REQUIRE(uart.OnWrite(0xE0001008, 0x04)); // Set TX_IRQ_EN (bit 2)

  // TX always ready, so IRQ should immediately assert
  REQUIRE(uart.HasIrq());

  // Clear IRQ
  uart.ClearIrq();

  // TX still ready, but IRQ cleared until next evaluation
  REQUIRE_FALSE(uart.HasIrq());
}

TEST_CASE("UART - CONTROL Register Read/Write") {
  UartDevice uart;
  Core::Data data;

  // Write to CONTROL
  REQUIRE(uart.OnWrite(0xE0001008, 0x0C)); // Set both IRQ enable bits

  // Read back
  REQUIRE(uart.OnRead(0xE0001008, data));
  REQUIRE(data == 0x0C);

  // Modify specific bits
  REQUIRE(uart.OnWrite(0xE0001008, 0x04)); // Only TX_IRQ_EN
  REQUIRE(uart.OnRead(0xE0001008, data));
  REQUIRE(data == 0x04);
}

TEST_CASE("UART - STATUS Register is Read-Only") {
  UartDevice uart;
  Core::Data data;

  // Read initial STATUS
  REQUIRE(uart.OnRead(0xE0001004, data));
  Core::Data initialStatus = data;

  // Attempt to write STATUS
  REQUIRE(uart.OnWrite(0xE0001004, 0xFF)); // Write all 1s

  // STATUS should be unchanged
  REQUIRE(uart.OnRead(0xE0001004, data));
  REQUIRE(data == initialStatus);
}

TEST_CASE("UART - Multiple RX Bytes (FIFO behavior)") {
  UartDevice uart;
  Core::Data data;

  // Queue multiple bytes
  uart.SimulateReceive(0x41); // 'A'
  uart.SimulateReceive(0x42); // 'B'
  uart.SimulateReceive(0x43); // 'C'

  // Read them in order (FIFO)
  REQUIRE(uart.OnRead(0xE0001000, data));
  REQUIRE(data == 0x41);

  REQUIRE(uart.OnRead(0xE0001000, data));
  REQUIRE(data == 0x42);

  REQUIRE(uart.OnRead(0xE0001000, data));
  REQUIRE(data == 0x43);

  // Buffer now empty
  REQUIRE(uart.OnRead(0xE0001000, data));
  REQUIRE(data == 0x00);
}

TEST_CASE("UART - Reserved Register Access") {
  UartDevice uart;
  Core::Data data;

  // Read from undefined offset (should return 0)
  REQUIRE(uart.OnRead(0xE0001100, data)); // Random offset
  REQUIRE(data == 0);

  // Write to undefined offset (should succeed silently)
  REQUIRE(uart.OnWrite(0xE0001200, 0xFF));
}
