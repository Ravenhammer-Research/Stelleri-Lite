#include "BridgeTableFormatter.hpp"
#include "ConfigurationManager.hpp"
#include "InterfaceTableFormatter.hpp"
#include "InterfaceToken.hpp"
#include "LaggTableFormatter.hpp"
#include "Parser.hpp"
#include "SingleInterfaceSummaryFormatter.hpp"
#include "TunnelTableFormatter.hpp"
#include "VLANTableFormatter.hpp"
#include "VirtualTableFormatter.hpp"
#include <iomanip>
#include <iostream>
#include <sstream>

void netcli::Parser::executeShowInterface(const InterfaceToken &tok,
                                          ConfigurationManager *mgr) const {
  if (!mgr) {
    std::cout << "No ConfigurationManager provided\n";
    return;
  }
  std::vector<ConfigData> interfaces;
  if (!tok.name().empty()) {
    // Prefer to query the ConfigurationManager for the named interface
    auto cdopt = mgr->getInterface(tok.name());
    if (cdopt) {
      if (cdopt->iface && cdopt->iface->type == InterfaceType::Bridge) {
        // Retrieve bridge-specific representation (members, etc.)
        auto brs = mgr->GetBridgeInterfaces();
        for (auto &b : brs) {
          if (b.name == tok.name()) {
            ConfigData cd;
            cd.iface = std::make_shared<BridgeInterfaceConfig>(std::move(b));
            interfaces.push_back(std::move(cd));
            break;
          }
        }
      } else {
        interfaces.push_back(std::move(*cdopt));
      }
    }
  } else if (tok.type() != InterfaceType::Unknown) {
    if (tok.type() == InterfaceType::Bridge) {
      auto brs = mgr->GetBridgeInterfaces();
      for (auto &b : brs) {
        ConfigData cd;
        cd.iface = std::make_shared<BridgeInterfaceConfig>(std::move(b));
        interfaces.push_back(std::move(cd));
      }
    } else if (tok.type() == InterfaceType::Lagg) {
      auto lgs = mgr->GetLaggInterfaces();
      for (auto &l : lgs) {
        ConfigData cd;
        cd.iface = std::make_shared<LaggConfig>(std::move(l));
        interfaces.push_back(std::move(cd));
      }
    } else {
      auto allIfaces = mgr->getInterfaces();
      for (auto &iface : allIfaces) {
        if (!iface.iface)
          continue;
        if (tok.type() == InterfaceType::Tunnel || tok.type() == InterfaceType::Gif || tok.type() == InterfaceType::Tun) {
          if (iface.iface->type == InterfaceType::Tunnel || iface.iface->type == InterfaceType::Gif || iface.iface->type == InterfaceType::Tun) {
            interfaces.push_back(std::move(iface));
          }
          continue;
        }

        if (iface.iface->type == tok.type())
          interfaces.push_back(std::move(iface));
      }
    }
  } else {
    interfaces = mgr->getInterfaces();
  }

  if (interfaces.size() == 1 && !tok.name().empty()) {
    SingleInterfaceSummaryFormatter formatter;
    std::cout << formatter.format(interfaces[0]);
    return;
  }

  bool allBridge = true;
  bool allLagg = true;
  bool allVlan = true;
  bool allTunnel = true;
  bool allVirtual = true;

  for (const auto &cd : interfaces) {
    if (!cd.iface)
      continue;
    if (cd.iface->type != InterfaceType::Bridge)
      allBridge = false;
    if (cd.iface->type != InterfaceType::Lagg)
      allLagg = false;
    if (cd.iface->type != InterfaceType::VLAN)
      allVlan = false;
    // Treat explicit tunnel-ish interface types as tunnels for formatting
    if (!(cd.iface->type == InterfaceType::Tunnel ||
        cd.iface->type == InterfaceType::Gif ||
        cd.iface->type == InterfaceType::Tun))
      allTunnel = false;
    if (cd.iface->type != InterfaceType::Virtual)
      allVirtual = false;
  }

  if (allBridge && !interfaces.empty()) {
    BridgeTableFormatter formatter;
    std::cout << formatter.format(interfaces);
    return;
  } else if (allLagg && !interfaces.empty()) {
    LaggTableFormatter formatter;
    std::cout << formatter.format(interfaces);
    return;
  } else if (allVlan && !interfaces.empty()) {
    VLANTableFormatter formatter;
    std::cout << formatter.format(interfaces);
    return;
  } else if (allTunnel && !interfaces.empty()) {
    TunnelTableFormatter formatter;
    std::cout << formatter.format(interfaces);
    return;
  } else if (allVirtual && !interfaces.empty()) {
    VirtualTableFormatter formatter;
    std::cout << formatter.format(interfaces);
    return;
  }

  InterfaceTableFormatter formatter;
  std::cout << formatter.format(interfaces);
}
