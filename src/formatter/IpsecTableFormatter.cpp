#include "IpsecTableFormatter.hpp"
#include "IpsecInterfaceConfig.hpp"
#include <string>
#include <vector>

std::string IpsecTableFormatter::format(
    const std::vector<IpsecInterfaceConfig> &interfaces) {
  if (interfaces.empty()) {
    return "No ipsec interfaces found.\n";
  }

  addColumn("Interface", "Interface", 10, 4, true);
  addColumn("Source", "Source", 5, 6, true);
  addColumn("Destination", "Destination", 5, 6, true);
  addColumn("VRF", "VRF", 5, 3, false);
  addColumn("Tunnel VRF", "Tunnel VRF", 4, 3, false);

  for (const auto &ipsec : interfaces) {
    std::string source = ipsec.source ? ipsec.source->toString() : "-";
    std::string destination =
        ipsec.destination ? ipsec.destination->toString() : "-";
    std::string vrfStr = ipsec.vrf ? std::to_string(ipsec.vrf->table) : "-";
    std::string tunnelVrfStr =
        ipsec.tunnel_vrf ? std::to_string(*ipsec.tunnel_vrf) : "-";

    addRow({ipsec.name, source, destination, vrfStr, tunnelVrfStr});
  }

  auto out = renderTable(80);
  out += "\n";
  return out;
}
