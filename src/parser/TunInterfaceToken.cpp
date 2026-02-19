/*
 * Copyright (c) 2026, Ravenhammer Research Inc.
 * All rights reserved.
 */

#include "ConfigurationManager.hpp"
#include "IPAddress.hpp"
#include "InterfaceToken.hpp"
#include "SingleTunSummaryFormatter.hpp"
#include "TunInterfaceConfig.hpp"
#include "TunTableFormatter.hpp"
#include <iostream>

class TunInterfaceToken : public InterfaceToken {
public:
  using InterfaceToken::InterfaceToken;
};

std::string InterfaceToken::toString(TunInterfaceConfig *cfg) {
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

bool InterfaceToken::parseTunKeywords(std::shared_ptr<InterfaceToken> &tok,
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
InterfaceToken::tunCompletions(const std::string &prev) {
  if (prev.empty())
    return {"source", "destination", "tunnel-vrf"};
  return {};
}

void InterfaceToken::setTunInterface(const InterfaceToken &tok,
                                     ConfigurationManager *mgr,
                                     InterfaceConfig &base, bool exists) {
  TunInterfaceConfig tc(base);
  if (tok.source)
    tc.source = IPAddress::fromString(*tok.source);
  if (tok.destination)
    tc.destination = IPAddress::fromString(*tok.destination);
  if (tok.tunnel_vrf)
    tc.tunnel_vrf = *tok.tunnel_vrf;
  tc.save(*mgr);
  std::cout << "set interface: " << (exists ? "updated" : "created") << " tun '"
            << tok.name() << "'\n";
}

bool InterfaceToken::showTunInterface(const InterfaceConfig &ic,
                                      ConfigurationManager *mgr) {
  std::vector<InterfaceConfig> v = {ic};
  auto tuns = mgr->GetTunInterfaces(v);
  if (!tuns.empty()) {
    SingleTunSummaryFormatter f;
    std::cout << f.format(tuns[0]);
    return true;
  }
  return false;
}

std::string
InterfaceToken::showTunInterfaces(const std::vector<InterfaceConfig> &ifaces,
                                  ConfigurationManager *mgr) {
  TunTableFormatter f;
  return f.format(mgr->GetTunInterfaces(ifaces));
}