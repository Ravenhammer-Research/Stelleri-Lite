/*
 * Copyright (c) 2026, Ravenhammer Research Inc.
 * All rights reserved.
 */

#include "CarpInterfaceConfig.hpp"
#include "CarpTableFormatter.hpp"
#include "ConfigurationManager.hpp"
#include "InterfaceToken.hpp"
#include "SingleCarpSummaryFormatter.hpp"
#include <iostream>

class CarpInterfaceToken : public InterfaceToken {
public:
  using InterfaceToken::InterfaceToken;
};

std::string InterfaceToken::toString(CarpInterfaceConfig *cfg) {
  if (!cfg)
    return std::string();
  std::string s = InterfaceToken::toString(static_cast<InterfaceConfig *>(cfg));
  if (cfg->vhid)
    s += " vhid " + std::to_string(*cfg->vhid);
  if (cfg->advskew)
    s += " advskew " + std::to_string(*cfg->advskew);
  if (cfg->advbase)
    s += " advbase " + std::to_string(*cfg->advbase);
  if (cfg->key)
    s += " key " + *cfg->key;
  return s;
}

bool InterfaceToken::parseCarpKeywords(std::shared_ptr<InterfaceToken> &tok,
                                       const std::vector<std::string> &tokens,
                                       size_t &cur) {
  const std::string &kw = tokens[cur];

  if (kw == "vhid" && cur + 1 < tokens.size()) {
    if (!tok->carp)
      tok->carp.emplace(InterfaceConfig{});
    tok->carp->vhid = std::stoi(tokens[cur + 1]);
    cur += 2;
    return true;
  }
  if (kw == "advskew" && cur + 1 < tokens.size()) {
    if (!tok->carp)
      tok->carp.emplace(InterfaceConfig{});
    tok->carp->advskew = std::stoi(tokens[cur + 1]);
    cur += 2;
    return true;
  }
  if (kw == "advbase" && cur + 1 < tokens.size()) {
    if (!tok->carp)
      tok->carp.emplace(InterfaceConfig{});
    tok->carp->advbase = std::stoi(tokens[cur + 1]);
    cur += 2;
    return true;
  }
  if (kw == "key" && cur + 1 < tokens.size()) {
    if (!tok->carp)
      tok->carp.emplace(InterfaceConfig{});
    tok->carp->key = tokens[cur + 1];
    cur += 2;
    return true;
  }
  return false;
}

std::vector<std::string>
InterfaceToken::carpCompletions(const std::string &prev) {
  if (prev.empty())
    return {"vhid", "advskew", "advbase", "key"};
  return {};
}

void InterfaceToken::setCarpInterface(const InterfaceToken &tok,
                                      ConfigurationManager *mgr,
                                      InterfaceConfig &base, bool exists) {
  CarpInterfaceConfig cc(base);
  if (tok.carp) {
    if (tok.carp->vhid)
      cc.vhid = tok.carp->vhid;
    if (tok.carp->advskew)
      cc.advskew = tok.carp->advskew;
    if (tok.carp->advbase)
      cc.advbase = tok.carp->advbase;
    if (tok.carp->key)
      cc.key = tok.carp->key;
  }
  cc.save(*mgr);
  std::cout << "set interface: " << (exists ? "updated" : "created")
            << " carp '" << tok.name() << "'\n";
}

bool InterfaceToken::showCarpInterface(const InterfaceConfig &ic,
                                       ConfigurationManager *mgr) {
  std::vector<InterfaceConfig> v = {ic};
  auto carps = mgr->GetCarpInterfaces(v);
  if (!carps.empty()) {
    SingleCarpSummaryFormatter f;
    std::cout << f.format(carps[0]);
    return true;
  }
  return false;
}

std::string
InterfaceToken::showCarpInterfaces(const std::vector<InterfaceConfig> &ifaces,
                                   ConfigurationManager *) {
  CarpTableFormatter f;
  return f.format(ifaces);
}
