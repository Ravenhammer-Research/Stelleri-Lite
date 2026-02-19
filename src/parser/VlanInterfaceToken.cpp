/*
 * Copyright (c) 2026, Ravenhammer Research Inc.
 * All rights reserved.
 */

#include "ConfigurationManager.hpp"
#include "InterfaceToken.hpp"
#include "PriorityCodePoint.hpp"
#include "SingleVlanSummaryFormatter.hpp"
#include "VlanInterfaceConfig.hpp"
#include "VlanTableFormatter.hpp"
#include <iostream>

class VlanInterfaceToken : public InterfaceToken {
public:
  using InterfaceToken::InterfaceToken;
};

std::string InterfaceToken::toString(VlanInterfaceConfig *cfg) {
  if (!cfg)
    return std::string();
  std::string s = InterfaceToken::toString(static_cast<InterfaceConfig *>(cfg));
  s += " vid " + std::to_string(cfg->id);
  if (cfg->parent)
    s += " parent " + *cfg->parent;
  if (cfg->pcp)
    s += " pcp " + std::to_string(static_cast<int>(*cfg->pcp));
  return s;
}

bool InterfaceToken::parseVlanKeywords(std::shared_ptr<InterfaceToken> &tok,
                                       const std::vector<std::string> &tokens,
                                       size_t &cur) {
  const std::string &kw = tokens[cur];

  // vlan sub-block: vlan id <N> parent <iface>
  if (kw == "vlan") {
    ++cur;
    std::optional<uint16_t> vid;
    std::optional<std::string> parent;
    while (cur < tokens.size()) {
      const std::string &k2 = tokens[cur];
      if (k2 == "id" && cur + 1 < tokens.size()) {
        vid = static_cast<uint16_t>(std::stoi(tokens[cur + 1]));
        cur += 2;
        continue;
      }
      if (k2 == "parent" && cur + 1 < tokens.size()) {
        parent = tokens[cur + 1];
        cur += 2;
        continue;
      }
      break;
    }
    if (vid && parent) {
      tok->vlan.emplace();
      tok->vlan->name = tok->name();
      tok->vlan->id = *vid;
      tok->vlan->parent = *parent;
    }
    return true;
  }

  if (kw == "vid" && cur + 1 < tokens.size()) {
    if (!tok->vlan) {
      tok->vlan.emplace();
      tok->vlan->name = tok->name();
    }
    tok->vlan->id = static_cast<uint16_t>(std::stoi(tokens[cur + 1]));
    cur += 2;
    return true;
  }

  if (kw == "parent" && cur + 1 < tokens.size()) {
    if (!tok->vlan) {
      tok->vlan.emplace();
      tok->vlan->name = tok->name();
    }
    tok->vlan->parent = tokens[cur + 1];
    cur += 2;
    return true;
  }

  if (kw == "pcp" && cur + 1 < tokens.size()) {
    if (!tok->vlan) {
      tok->vlan.emplace();
      tok->vlan->name = tok->name();
    }
    tok->vlan->pcp = static_cast<PriorityCodePoint>(std::stoi(tokens[cur + 1]));
    cur += 2;
    return true;
  }

  return false;
}

std::vector<std::string>
InterfaceToken::vlanCompletions(const std::string &prev) {
  if (prev.empty())
    return {"vid", "parent", "vlan", "pcp"};
  return {};
}

void InterfaceToken::setVlanInterface(const InterfaceToken &tok,
                                      ConfigurationManager *mgr,
                                      InterfaceConfig &base, bool exists) {
  if (!tok.vlan || tok.vlan->id == 0 || !tok.vlan->parent) {
    std::cerr << "set interface: VLAN creation requires VLAN id and parent "
                 "interface.\n"
              << "Usage: set interface name <vlan_name> vlan id <vlan_id> "
                 "parent <parent_iface>\n";
    return;
  }
  VlanInterfaceConfig vc(base, tok.vlan->id, tok.vlan->parent, tok.vlan->pcp);
  vc.InterfaceConfig::name = tok.name();
  vc.save(*mgr);
  std::cout << "set interface: " << (exists ? "updated" : "created")
            << " vlan '" << tok.name() << "'\n";
}

bool InterfaceToken::showVlanInterface(const InterfaceConfig &ic,
                                       ConfigurationManager *mgr) {
  std::vector<InterfaceConfig> v = {ic};
  auto vlans = mgr->GetVLANInterfaces(v);
  if (!vlans.empty()) {
    SingleVlanSummaryFormatter f;
    std::cout << f.format(vlans[0]);
    return true;
  }
  return false;
}

std::string
InterfaceToken::showVlanInterfaces(const std::vector<InterfaceConfig> &ifaces,
                                   ConfigurationManager *mgr) {
  VlanTableFormatter f;
  return f.format(mgr->GetVLANInterfaces(ifaces));
}
