/**
 * Symbol Table.
 *
 * Stores the mapping between label names and their memory addresses.
 *
 * Author: KleaSCM
 * Email: KleaSCM@gmail.com
 */

#pragma once

#include "Core/Types.hpp"
#include <optional>
#include <string>
#include <unordered_map>

namespace Aurelia::Tools::Assembler {

using Address = Core::Address;

class SymbolTable {
public:
  void Define(const std::string &name, Address address) {
    m_Symbols[name] = address;
  }

  [[nodiscard]] std::optional<Address> Resolve(const std::string &name) const {
    auto it = m_Symbols.find(name);
    if (it != m_Symbols.end()) {
      return it->second;
    }
    return std::nullopt;
  }

  [[nodiscard]] bool Contains(const std::string &name) const {
    return m_Symbols.find(name) != m_Symbols.end();
  }

private:
  std::unordered_map<std::string, Address> m_Symbols;
};

} // namespace Aurelia::Tools::Assembler
