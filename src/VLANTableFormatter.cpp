#include "VLANTableFormatter.hpp"
#include "InterfaceConfig.hpp"
#include "InterfaceFlags.hpp"
#include "InterfaceType.hpp"
#include "VLANConfig.hpp"
#include "VLANFlags.hpp"
#include "VLANProto.hpp"
#include <algorithm>
#include <iomanip>
#include <sstream>

static std::string vlanProtoToString(const std::optional<VLANProto> &p) {
  if (!p) return std::string("-");
  switch (*p) {
  case VLANProto::DOT1Q:
    return std::string("802.1q");
  case VLANProto::DOT1AD:
    return std::string("802.1ad");
  case VLANProto::UNKNOWN:
    return std::string("unknown");
  default:
    return std::string("other");
  }
}

// Convert raw IFCAP_* capability bits into a human-readable comma-separated
// string. Per project style this rendering belongs in the formatter.
static std::string vlanCapsToString(uint32_t mask) {
  std::string out;
  using netcli::VLANFlag;
  if (netcli::has_flag(mask, VLANFlag::RXCSUM)) {
    if (!out.empty()) out += ",";
    out += "RXCSUM";
  }
  if (netcli::has_flag(mask, VLANFlag::TXCSUM)) {
    if (!out.empty()) out += ",";
    out += "TXCSUM";
  }
  if (netcli::has_flag(mask, VLANFlag::LINKSTATE)) {
    if (!out.empty()) out += ",";
    out += "LINKSTATE";
  }
  if (netcli::has_flag(mask, VLANFlag::VLAN_HWTAG)) {
    if (!out.empty()) out += ",";
    out += "VLAN_HWTAG";
  }
  return out;
}

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
    std::string protoStr = (ic.vlan ? vlanProtoToString(ic.vlan->proto) : std::string("-"));
    std::string flagsStr = (ic.flags ? flagsToString(*ic.flags) : std::string("-"));
    std::string optionsStr = "-";
    if (ic.vlan && ic.vlan->options_bits) {
      auto s = vlanCapsToString(*ic.vlan->options_bits);
      if (!s.empty()) optionsStr = s;
    }

    // Add an Options column (capability flags from SIOCGIFCAP)
    oss << std::left << std::setw(15) << ic.name << std::setw(8) << vidStr
      << std::setw(12) << nameStr << std::setw(15) << parent << std::setw(6)
      << pcpStr << std::setw(8)
      << (ic.mtu ? std::to_string(*ic.mtu) : std::string("-")) << std::setw(8)
      << flagsStr << std::setw(12) << protoStr << std::setw(20) << optionsStr << "\n";
  }

  oss << "\n";
  return oss.str();
}
