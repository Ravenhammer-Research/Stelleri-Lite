/*
 * Copyright (c) 2026, Ravenhammer Research Inc.
 * All rights reserved.
 */

#include "ConfigurationManager.hpp"
#include "InterfaceToken.hpp"
#include "SingleVxlanSummaryFormatter.hpp"
#include "VxlanInterfaceConfig.hpp"
#include "VxlanTableFormatter.hpp"
#include <iostream>

class VxlanInterfaceToken : public InterfaceToken {
public:
  using InterfaceToken::InterfaceToken;
};

std::string InterfaceToken::toString(VxlanInterfaceConfig *cfg) {
  if (!cfg)
    return std::string();
  std::string s = InterfaceToken::toString(static_cast<InterfaceConfig *>(cfg));
  if (cfg->vni)
    s += " vni " + std::to_string(*cfg->vni);
  if (cfg->localAddr)
    s += " local " + *cfg->localAddr;
  if (cfg->remoteAddr)
    s += " remote " + *cfg->remoteAddr;
  if (cfg->localPort)
    s += " port " + std::to_string(*cfg->localPort);
  return s;
}

bool InterfaceToken::parseVxlanKeywords(std::shared_ptr<InterfaceToken> &tok,
                                        const std::vector<std::string> &tokens,
                                        size_t &cur) {
  const std::string &kw = tokens[cur];

  if (kw == "vni" && cur + 1 < tokens.size()) {
    if (!tok->vxlan)
      tok->vxlan.emplace(InterfaceConfig{});
    tok->vxlan->vni = static_cast<uint32_t>(std::stoul(tokens[cur + 1]));
    cur += 2;
    return true;
  }
  if (kw == "local" && cur + 1 < tokens.size()) {
    if (!tok->vxlan)
      tok->vxlan.emplace(InterfaceConfig{});
    tok->vxlan->localAddr = tokens[cur + 1];
    cur += 2;
    return true;
  }
  if (kw == "remote" && cur + 1 < tokens.size()) {
    if (!tok->vxlan)
      tok->vxlan.emplace(InterfaceConfig{});
    tok->vxlan->remoteAddr = tokens[cur + 1];
    cur += 2;
    return true;
  }
  if (kw == "port" && cur + 1 < tokens.size()) {
    if (!tok->vxlan)
      tok->vxlan.emplace(InterfaceConfig{});
    tok->vxlan->localPort = static_cast<uint16_t>(std::stoi(tokens[cur + 1]));
    cur += 2;
    return true;
  }
  return false;
}

std::vector<std::string>
InterfaceToken::vxlanCompletions(const std::string &prev) {
  if (prev.empty())
    return {"vni", "local", "remote", "port"};
  return {};
}

void InterfaceToken::setVxlanInterface(const InterfaceToken &tok,
                                       ConfigurationManager *mgr,
                                       InterfaceConfig &base, bool exists) {
  VxlanInterfaceConfig vxc(base);
  if (tok.vxlan) {
    if (tok.vxlan->vni)
      vxc.vni = tok.vxlan->vni;
    if (tok.vxlan->localAddr)
      vxc.localAddr = tok.vxlan->localAddr;
    if (tok.vxlan->remoteAddr)
      vxc.remoteAddr = tok.vxlan->remoteAddr;
    if (tok.vxlan->localPort)
      vxc.localPort = tok.vxlan->localPort;
  }
  vxc.save(*mgr);
  std::cout << "set interface: " << (exists ? "updated" : "created")
            << " vxlan '" << tok.name() << "'\n";
}

bool InterfaceToken::showVxlanInterface(const InterfaceConfig &ic,
                                        ConfigurationManager *mgr) {
  std::vector<InterfaceConfig> v = {ic};
  auto vxlans = mgr->GetVxlanInterfaces(v);
  if (!vxlans.empty()) {
    SingleVxlanSummaryFormatter f;
    std::cout << f.format(vxlans[0]);
    return true;
  }
  return false;
}

std::string
InterfaceToken::showVxlanInterfaces(const std::vector<InterfaceConfig> &ifaces,
                                    ConfigurationManager *mgr) {
  VxlanTableFormatter f;
  return f.format(mgr->GetVxlanInterfaces(ifaces));
}
