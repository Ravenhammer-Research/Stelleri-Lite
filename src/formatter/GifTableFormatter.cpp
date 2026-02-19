#include "GifTableFormatter.hpp"
#include "GifInterfaceConfig.hpp"
#include <string>
#include <vector>

std::string
GifTableFormatter::format(const std::vector<GifInterfaceConfig> &interfaces) {
  if (interfaces.empty()) {
    return "No gif interfaces found.\n";
  }

  addColumn("Interface", "Interface", 10, 4, true);
  addColumn("Source", "Source", 5, 6, true);
  addColumn("Destination", "Destination", 5, 6, true);
  addColumn("VRF", "VRF", 5, 3, false);
  addColumn("Tunnel VRF", "Tunnel VRF", 4, 3, false);

  for (const auto &gif : interfaces) {
    std::string source = gif.source ? gif.source->toString() : "-";
    std::string destination =
        gif.destination ? gif.destination->toString() : "-";
    std::string vrfStr = gif.vrf ? std::to_string(gif.vrf->table) : "-";
    std::string tunnelVrfStr =
        gif.tunnel_vrf ? std::to_string(*gif.tunnel_vrf) : "-";

    addRow({gif.name, source, destination, vrfStr, tunnelVrfStr});
  }

  auto out = renderTable(80);
  out += "\n";
  return out;
}
