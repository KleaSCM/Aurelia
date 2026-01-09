/**
 * Bus Definitions.
 *
 * Defines the signals and state of the synchronous system bus.
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#pragma once

#include "Core/Types.hpp"

namespace Aurelia::Bus {

enum class ControlSignal : Core::Byte {
  None = 0,
  Read = 1 << 0,  // Master requests Read
  Write = 1 << 1, // Master requests Write
  Wait = 1 << 2,  // Slave holds the bus (Busy)
  Ready = 1 << 3, // Slave is ready to transfer (Ack)
  Irq = 1 << 4    // Interrupt Request
};

struct BusState {
  Core::Address AddrBus = 0; // Address Lines
  Core::Data DataBus = 0;    // Data Lines
  Core::Byte Control = 0;    // Control Lines (Bitmask of ControlSignal)
};

} // namespace Aurelia::Bus
