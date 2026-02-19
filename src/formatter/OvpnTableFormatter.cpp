#include "OvpnTableFormatter.hpp"
#include "OvpnInterfaceConfig.hpp"
#include <string>
#include <vector>

std::string
OvpnTableFormatter::format(const std::vector<OvpnInterfaceConfig> &interfaces) {
  if (interfaces.empty()) {
    return "No ovpn interfaces found.\n";
  }

  addColumn("Interface", "Interface", 10, 4, true);
  addColumn("Source", "Source", 5, 6, true);
  addColumn("Destination", "Destination", 5, 6, true);
  addColumn("VRF", "VRF", 5, 3, false);
  addColumn("Tunnel VRF", "Tunnel VRF", 4, 3, false);

  for (const auto &ovpn : interfaces) {
    std::string source = ovpn.source ? ovpn.source->toString() : "-";
    std::string destination =
        ovpn.destination ? ovpn.destination->toString() : "-";
    std::string vrfStr = ovpn.vrf ? std::to_string(ovpn.vrf->table) : "-";
    std::string tunnelVrfStr =
        ovpn.tunnel_vrf ? std::to_string(*ovpn.tunnel_vrf) : "-";

    addRow({ovpn.name, source, destination, vrfStr, tunnelVrfStr});
  }

  auto out = renderTable(80);
  out += "\n";
  return out;
}
