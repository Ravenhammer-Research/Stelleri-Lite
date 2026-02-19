/*
 * Copyright (c) 2026, Ravenhammer Research Inc.
 * All rights reserved.
 */

#include "ConfigurationManager.hpp"
#include "IPAddress.hpp"
#include "InterfaceToken.hpp"
#include "OvpnInterfaceConfig.hpp"
#include "OvpnTableFormatter.hpp"
#include "SingleOvpnSummaryFormatter.hpp"
#include <iostream>

class OvpnInterfaceToken : public InterfaceToken {
public:
  using InterfaceToken::InterfaceToken;
};

std::string InterfaceToken::toString(OvpnInterfaceConfig *cfg) {
  if (!cfg)
    return std::string();
  std::string s = InterfaceToken::toString(static_cast<InterfaceConfig *>(cfg));
  if (cfg->source)
    s += " source " + cfg->source->toString();
  if (cfg->destination)
    s += " destination " + cfg->destination->toString();
  if (cfg->tunnel_vrf)
    s += " tunnel-vrf " + std::to_string(*cfg->tunnel_vrf);
  return s;
}

bool InterfaceToken::parseOvpnKeywords(std::shared_ptr<InterfaceToken> &tok,
                                       const std::vector<std::string> &tokens,
                                       size_t &cur) {
  const std::string &kw = tokens[cur];

  if (kw == "source" && cur + 1 < tokens.size()) {
    tok->source = tokens[cur + 1];
    cur += 2;
    return true;
  }
  if (kw == "destination" && cur + 1 < tokens.size()) {
    tok->destination = tokens[cur + 1];
    cur += 2;
    return true;
  }
  if ((kw == "tunnel-vrf" || kw == "tunnel-fib") && cur + 1 < tokens.size()) {
    tok->tunnel_vrf = std::stoi(tokens[cur + 1]);
    cur += 2;
    return true;
  }
  return false;
}

std::vector<std::string>
InterfaceToken::ovpnCompletions(const std::string &prev) {
  if (prev.empty())
    return {"source", "destination", "tunnel-vrf"};
  return {};
}

void InterfaceToken::setOvpnInterface(const InterfaceToken &tok,
                                      ConfigurationManager *mgr,
                                      InterfaceConfig &base, bool exists) {
  OvpnInterfaceConfig oc(base);
  if (tok.source)
    oc.source = IPAddress::fromString(*tok.source);
  if (tok.destination)
    oc.destination = IPAddress::fromString(*tok.destination);
  if (tok.tunnel_vrf)
    oc.tunnel_vrf = *tok.tunnel_vrf;
  oc.save(*mgr);
  std::cout << "set interface: " << (exists ? "updated" : "created")
            << " ovpn '" << tok.name() << "'\n";
}

bool InterfaceToken::showOvpnInterface(const InterfaceConfig &ic,
                                       ConfigurationManager *mgr) {
  std::vector<InterfaceConfig> v = {ic};
  auto ovpns = mgr->GetOvpnInterfaces(v);
  if (!ovpns.empty()) {
    SingleOvpnSummaryFormatter f;
    std::cout << f.format(ovpns[0]);
    return true;
  }
  return false;
}

std::string
InterfaceToken::showOvpnInterfaces(const std::vector<InterfaceConfig> &ifaces,
                                   ConfigurationManager *mgr) {
  OvpnTableFormatter f;
  return f.format(mgr->GetOvpnInterfaces(ifaces));
}