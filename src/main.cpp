/**
 * Aurelia Virtual Machine Entry Point.
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#include "Core/BitManip.hpp"
#include "Core/Types.hpp"
#include <iostream>

int main() {
  // Simple boot message for verification
  std::cout << "Aurelia Virtual System Initializing..." << "\n";
  std::cout << "Arch: 64-bit" << "\n";

  // Basic usage check of Types
  Aurelia::Core::Data data = 0xDEADBEEF;

  if (Aurelia::Core::CheckBit(data, 0)) {
    std::cout << "Bit 0 is set!" << "\n";
  }

  std::cout << "System Halted." << "\n";
  return 0;
}
