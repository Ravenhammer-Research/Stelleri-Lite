/*
 * Copyright (c) 2026, Ravenhammer Research Inc.
 * All rights reserved.
 */

#include "BridgeInterfaceConfig.hpp"
#include "BridgeTableFormatter.hpp"
#include "ConfigurationManager.hpp"
#include "InterfaceToken.hpp"
#include "SingleBridgeSummaryFormatter.hpp"
#include <iostream>

class BridgeInterfaceToken : public InterfaceToken {
public:
  using InterfaceToken::InterfaceToken;
};

std::string InterfaceToken::toString(BridgeInterfaceConfig *cfg) {
  if (!cfg)
    return std::string();
  std::string s = InterfaceToken::toString(static_cast<InterfaceConfig *>(cfg));
  for (const auto &m : cfg->members)
    s += " member " + m;
  if (cfg->stp)
    s += " stp on";
  if (cfg->priority)
    s += " priority " + std::to_string(*cfg->priority);
  return s;
}

bool InterfaceToken::parseBridgeKeywords(std::shared_ptr<InterfaceToken> &tok,
                                         const std::vector<std::string> &tokens,
                                         size_t &cur) {
  const std::string &kw = tokens[cur];

  if (kw == "member" && cur + 1 < tokens.size()) {
    if (!tok->bridge)
      tok->bridge.emplace();
    tok->bridge->members.push_back(tokens[cur + 1]);
    cur += 2;
    return true;
  }
  if (kw == "stp" && cur + 1 < tokens.size()) {
    if (!tok->bridge)
      tok->bridge.emplace();
    const std::string &val = tokens[cur + 1];
    tok->bridge->stp =
        (val == "on" || val == "yes" || val == "true" || val == "enable");
    cur += 2;
    return true;
  }
  if (kw == "priority" && cur + 1 < tokens.size()) {
    if (!tok->bridge)
      tok->bridge.emplace();
    tok->bridge->priority = std::stoi(tokens[cur + 1]);
    cur += 2;
    return true;
  }
  return false;
}

std::vector<std::string>
InterfaceToken::bridgeCompletions(const std::string &prev) {
  if (prev.empty())
    return {"member", "stp", "priority"};
  if (prev == "stp")
    return {"on", "off"};
  return {};
}

void InterfaceToken::setBridgeInterface(const InterfaceToken &tok,
                                        ConfigurationManager *mgr,
                                        InterfaceConfig &base, bool exists) {
  BridgeInterfaceConfig bic(base);
  if (tok.bridge) {
    for (const auto &m : tok.bridge->members)
      bic.members.push_back(m);
    bic.stp = tok.bridge->stp;
    if (tok.bridge->priority)
      bic.priority = tok.bridge->priority;
  }
  bic.save(*mgr);
  std::cout << "set interface: " << (exists ? "updated" : "created")
            << " bridge '" << tok.name() << "'\n";
}

bool InterfaceToken::showBridgeInterface(const InterfaceConfig &ic,
                                         ConfigurationManager *mgr) {
  std::vector<InterfaceConfig> v = {ic};
  auto bridges = mgr->GetBridgeInterfaces(v);
  if (!bridges.empty()) {
    SingleBridgeSummaryFormatter f;
    std::cout << f.format(bridges[0]);
    return true;
  }
  return false;
}

std::string
InterfaceToken::showBridgeInterfaces(const std::vector<InterfaceConfig> &ifaces,
                                     ConfigurationManager *mgr) {
  BridgeTableFormatter f;
  return f.format(mgr->GetBridgeInterfaces(ifaces));
}
