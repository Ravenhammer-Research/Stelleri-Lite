#include "LoopBackTableFormatter.hpp"
#include "InterfaceConfig.hpp"
#include "ConfigData.hpp"
#include <sstream>
#include <iomanip>

std::string LoopBackTableFormatter::format(
    const std::vector<ConfigData> &items) const {
  std::ostringstream oss;
  // Header
  oss << std::left << std::setw(12) << "Interface" << std::setw(18)
      << "Address" << std::setw(8) << "Status" << "\n";
  oss << std::string(12, '-') << std::string(18, '-') << std::string(8, '-')
      << "\n";

  for (const auto &cd : items) {
    if (!cd.iface) continue;
    const auto &ic = *cd.iface;
    if (ic.type != InterfaceType::Loopback) continue;

    std::string addr = "-";
    if (ic.address) addr = ic.address->toString();

    std::string status = "-";
    if (ic.flags) {
      if (*ic.flags & IFF_RUNNING) status = "active";
      else if (*ic.flags & IFF_UP) status = "no-carrier";
      else status = "down";
    }

    oss << std::left << std::setw(12) << ic.name << std::setw(18) << addr
        << std::setw(8) << status << "\n";

    // print aliases
    for (const auto &a : ic.aliases) {
      if (a) oss << std::left << std::setw(12) << "" << std::setw(18)
                  << a->toString() << std::setw(8) << "" << "\n";
    }
  }

  return oss.str();
}
