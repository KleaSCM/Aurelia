/**
 * ITickable Interface.
 *
 * Abstract base class for all components that must synchronize with the system
 * clock.
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#pragma once

namespace Aurelia::Core {

class ITickable {
public:
  virtual ~ITickable() = default;

  /**
   * Called once per system clock cycle.
   * Components should perform their synchronous logic here.
   */
  virtual void OnTick() = 0;
};

} // namespace Aurelia::Core
