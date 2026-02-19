/*
 * Copyright (c) 2026, Ravenhammer Research Inc.
 * All rights reserved.
 */

#include "ConfigurationManager.hpp"
#include "InterfaceToken.hpp"
#include "WireGuardInterfaceConfig.hpp"
#include "WireGuardTableFormatter.hpp"
#include <iostream>

class WireGuardInterfaceToken : public InterfaceToken {
public:
  using InterfaceToken::InterfaceToken;
};

std::string InterfaceToken::toString(WireGuardInterfaceConfig *cfg) {
  if (!cfg)
    return std::string();
  std::string s = InterfaceToken::toString(static_cast<InterfaceConfig *>(cfg));
  if (cfg->listenPort)
    s += " listen-port " + std::to_string(*cfg->listenPort);
  return s;
}

bool InterfaceToken::parseWireGuardKeywords(
    std::shared_ptr<InterfaceToken> &tok,
    const std::vector<std::string> &tokens, size_t &cur) {
  const std::string &kw = tokens[cur];

  if (kw == "listen-port" && cur + 1 < tokens.size()) {
    tok->wg_listen_port = static_cast<uint16_t>(std::stoi(tokens[cur + 1]));
    cur += 2;
    return true;
  }
  return false;
}

std::vector<std::string>
InterfaceToken::wireGuardCompletions(const std::string &prev) {
  if (prev.empty())
    return {"listen-port"};
  return {};
}

void InterfaceToken::setWireGuardInterface(const InterfaceToken &tok,
                                           ConfigurationManager *mgr,
                                           InterfaceConfig &base, bool exists) {
  WireGuardInterfaceConfig wgc(base);
  if (tok.wg_listen_port)
    wgc.listenPort = *tok.wg_listen_port;
  wgc.save(*mgr);
  std::cout << "set interface: " << (exists ? "updated" : "created")
            << " wireguard '" << tok.name() << "'\n";
}

std::string InterfaceToken::showWireGuardInterfaces(
    const std::vector<InterfaceConfig> &ifaces, ConfigurationManager *) {
  WireGuardTableFormatter f;
  return f.format(ifaces);
}
