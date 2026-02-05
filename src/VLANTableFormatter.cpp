#include "VLANTableFormatter.hpp"
#include "InterfaceConfig.hpp"
#include "InterfaceFlags.hpp"
#include "InterfaceType.hpp"
#include "VLANConfig.hpp"
#include <algorithm>
#include <iomanip>
#include <sstream>

std::string
VLANTableFormatter::format(const std::vector<ConfigData> &interfaces) const {
  if (interfaces.empty()) {
    return "No VLAN interfaces found.\n";
  }

  std::ostringstream oss;

  // Table header (VLAN-specific: include VLAN name and flags)
  oss << std::left << std::setw(15) << "Interface" << std::setw(8) << "VLAN ID"
      << std::setw(12) << "Name" << std::setw(15) << "Parent" << std::setw(6)
      << "PCP" << std::setw(8) << "MTU" << std::setw(8) << "Flags"
      << "\n";
  oss << std::string(72, '-') << "\n";

  for (const auto &cd : interfaces) {
    if (!cd.iface || cd.iface->type != InterfaceType::VLAN)
      continue;

    const auto &ic = *cd.iface;

    // Try to use explicit VLAN payload if present; otherwise derive from name
    int vid = -1;
    std::string parent = "-";
    std::optional<int> pcp;
    if (ic.vlan) {
      vid = ic.vlan->id;
      parent = ic.vlan->parent ? *ic.vlan->parent : std::string("-");
      if (ic.vlan->pcp)
        pcp = static_cast<int>(*ic.vlan->pcp);
    } else {
      // Attempt to parse forms like "re0.25" (parent.name notation).
      auto pos = ic.name.find('.');
      if (pos != std::string::npos) {
        parent = ic.name.substr(0, pos);
        std::string rest = ic.name.substr(pos + 1);
        try {
          vid = std::stoi(rest);
        } catch (...) {
          vid = -1;
        }
      }
    }

    std::string vidStr = (vid >= 0) ? std::to_string(vid) : std::string("-");
    std::string pcpStr =
        pcp ? std::to_string(static_cast<int>(*pcp)) : std::string("-");
    std::string nameStr =
        (ic.vlan && ic.vlan->name) ? *ic.vlan->name : std::string("-");
    std::string flagsStr =
        (ic.flags ? flagsToString(*ic.flags) : std::string("-"));

    oss << std::left << std::setw(15) << ic.name << std::setw(8) << vidStr
        << std::setw(12) << nameStr << std::setw(15) << parent << std::setw(6)
        << pcpStr << std::setw(8)
        << (ic.mtu ? std::to_string(*ic.mtu) : std::string("-")) << std::setw(8)
        << flagsStr << "\n";
  }

  oss << "\n";
  return oss.str();
}
