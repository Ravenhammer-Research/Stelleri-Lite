#include "BridgeInterfaceConfig.hpp"
#include "ConfigurationManager.hpp"
#include "InterfaceConfig.hpp"
#include "InterfaceToken.hpp"
#include "LaggConfig.hpp"
#include "LoopBackConfig.hpp"
#include "Parser.hpp"
#include "TunnelConfig.hpp"
#include "VLANConfig.hpp"
#include "VirtualInterfaceConfig.hpp"
#include <iostream>

void netcli::Parser::executeSetInterface(const InterfaceToken &tok,
                                         ConfigurationManager *mgr) const {
  (void)mgr;

  const std::string name = tok.name();
  if (name.empty()) {
    std::cerr << "set interface: missing interface name\n";
    return;
  }

  if (InterfaceConfig::exists(name)) {
    std::cout << "set interface: interface '" << name << "' already exists\n";
    return;
  }

  // Decide which concrete config to create based on token type or name prefix
  auto itype = tok.type();
  try {
    InterfaceConfig base;
    base.name = name;

    // If the token requested a specific VRF/FIB table, apply it to the base
    if (tok.vrf) {
      try {
        int tbl = std::stoi(*tok.vrf);
        if (!base.vrf) base.vrf = std::make_unique<VRFConfig>();
        base.vrf->table = tbl;
        base.vrf->name = std::string("fib") + std::to_string(tbl);
      } catch (...) {
        // ignore invalid fib value
      }
    }

    if (itype == InterfaceType::Bridge) {
      BridgeInterfaceConfig bic(base);
      bic.save();
      std::cout << "set interface: created bridge '" << name << "'\n";
      return;
    }

    if (itype == InterfaceType::Lagg) {
      // LAGG creation generally requires member interfaces; advise if absent.
      if (!tok.lagg || tok.lagg->members.empty()) {
        std::cerr << "set interface: LAGG creation typically requires member interfaces.\n"
                  << "Usage: set interface name <lagg_name> lagg members <if1,if2,...> [protocol <proto>]\n";
        return;
      }
      LaggConfig lac(base, tok.lagg->protocol, tok.lagg->members,
                      tok.lagg->hash_policy, tok.lagg->lacp_rate,
                      tok.lagg->min_links);
      lac.save();
      std::cout << "set interface: created lagg '" << name << "'\n";
      return;
    }

    if (itype == InterfaceType::VLAN) {
      // VLAN creation requires VLAN id and parent interface.
      if (!tok.vlan || tok.vlan->id == 0 || !tok.vlan->parent) {
        std::cerr << "set interface: VLAN creation requires VLAN id and parent interface.\n"
                  << "Usage: set interface name <vlan_name> vlan id <vlan_id> parent <parent_iface>\n";
        return;
      }

      VLANConfig vc(base, tok.vlan->id, tok.vlan->name, tok.vlan->parent,
            tok.vlan->pcp);
      // Ensure the underlying InterfaceConfig::name is set (constructor
      // initializes the optional VLAN name but may not set the inherited
      // InterfaceConfig::name field on all builds).
      vc.InterfaceConfig::name = name;
      vc.save();
      std::cout << "set interface: created vlan '" << name << "'\n";
      return;
    }

    if (itype == InterfaceType::Tunnel || itype == InterfaceType::Gif || itype == InterfaceType::Tun) {
      TunnelConfig tc(base);
      tc.save();
      std::cout << "set interface: created tunnel '" << name << "'\n";
      return;
    }

    if (itype == InterfaceType::Loopback) {
      // create a minimal loopback interface
      LoopBackConfig lbc(base);
      lbc.save();
      std::cout << "set interface: created loopback '" << name << "'\n";
      return;
    }

    // If no explicit type provided, infer from common name prefix (lo*)
    if (itype == InterfaceType::Unknown) {
      if (name.rfind("lo", 0) == 0) {
        LoopBackConfig lbc(base);
        lbc.save();
        std::cout << "set interface: created loopback '" << name << "'\n";
        return;
      }
    }

    if (itype == InterfaceType::Virtual) {
      VirtualInterfaceConfig vic(base);
      vic.save();
      std::cout << "set interface: created virtual iface '" << name << "'\n";
      return;
    }

    std::cerr << "set interface: unsupported or unknown interface type for '" << name << "'\n";
  } catch (const std::exception &e) {
    std::cerr << "set interface: failed to create '" << name << "': " << e.what() << "\n";
  }
}
