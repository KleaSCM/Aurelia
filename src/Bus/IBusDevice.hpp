/**
 * IBusDevice Interface.
 *
 * Contract for any component attaching to the system bus (RAM, CPU, IO).
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#pragma once

#include "Core/ITickable.hpp"
#include "Core/Types.hpp"

namespace Aurelia::Bus {

class IBusDevice : public Core::ITickable {
public:
  /**
   * Checks if the given physical address maps to this device.
   */
  [[nodiscard]] virtual bool IsAddressInRange(Core::Address addr) const = 0;

  /**
   * Called by the Bus when a READ operation is requested for this device.
   * The device should place data onto the outData reference.
   *
   * @return true if Read was serviced, false if device needs to WAIT.
   */
  virtual bool OnRead(Core::Address addr, Core::Data &outData) = 0;

  /**
   * Called by the Bus when a WRITE operation is requested.
   *
   * @return true if Write was completed, false if device needs to WAIT.
   */
  virtual bool OnWrite(Core::Address addr, Core::Data inData) = 0;
};

} // namespace Aurelia::Bus
