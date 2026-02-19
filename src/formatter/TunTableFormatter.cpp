#include "TunTableFormatter.hpp"
#include "TunInterfaceConfig.hpp"
#include <string>
#include <vector>

std::string
TunTableFormatter::format(const std::vector<TunInterfaceConfig> &interfaces) {
  if (interfaces.empty()) {
    return "No tun interfaces found.\n";
  }

  addColumn("Interface", "Interface", 10, 4, true);
  addColumn("Source", "Source", 5, 6, true);
  addColumn("Destination", "Destination", 5, 6, true);
  addColumn("VRF", "VRF", 5, 3, false);
  addColumn("Tunnel VRF", "Tunnel VRF", 4, 3, false);

  for (const auto &tun : interfaces) {
    std::string source = tun.source ? tun.source->toString() : "-";
    std::string destination =
        tun.destination ? tun.destination->toString() : "-";
    std::string vrfStr = tun.vrf ? std::to_string(tun.vrf->table) : "-";
    std::string tunnelVrfStr =
        tun.tunnel_vrf ? std::to_string(*tun.tunnel_vrf) : "-";

    addRow({tun.name, source, destination, vrfStr, tunnelVrfStr});
  }

  auto out = renderTable(80);
  out += "\n";
  return out;
}
