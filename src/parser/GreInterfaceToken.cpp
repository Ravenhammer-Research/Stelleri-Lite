/*
 * Copyright (c) 2026, Ravenhammer Research Inc.
 * All rights reserved.
 */

#include "ConfigurationManager.hpp"
#include "GreInterfaceConfig.hpp"
#include "GreTableFormatter.hpp"
#include "InterfaceToken.hpp"
#include "SingleGreSummaryFormatter.hpp"
#include <iostream>

class GreInterfaceToken : public InterfaceToken {
public:
  using InterfaceToken::InterfaceToken;
};

std::string InterfaceToken::toString(GreInterfaceConfig *cfg) {
  if (!cfg)
    return std::string();
  std::string s = InterfaceToken::toString(static_cast<InterfaceConfig *>(cfg));
  if (cfg->greSource)
    s += " source " + *cfg->greSource;
  if (cfg->greDestination)
    s += " destination " + *cfg->greDestination;
  if (cfg->greKey)
    s += " key " + std::to_string(*cfg->greKey);
  return s;
}

bool InterfaceToken::parseGreKeywords(std::shared_ptr<InterfaceToken> &tok,
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
  if (kw == "key" && cur + 1 < tokens.size()) {
    if (!tok->gre)
      tok->gre.emplace(InterfaceConfig{});
    tok->gre->greKey = static_cast<uint32_t>(std::stoul(tokens[cur + 1]));
    cur += 2;
    return true;
  }
  return false;
}

std::vector<std::string>
InterfaceToken::greCompletions(const std::string &prev) {
  if (prev.empty())
    return {"source", "destination", "key"};
  return {};
}

void InterfaceToken::setGreInterface(const InterfaceToken &tok,
                                     ConfigurationManager *mgr,
                                     InterfaceConfig &base, bool exists) {
  GreInterfaceConfig gc(base);
  if (tok.source)
    gc.greSource = *tok.source;
  if (tok.destination)
    gc.greDestination = *tok.destination;
  if (tok.gre && tok.gre->greKey)
    gc.greKey = tok.gre->greKey;
  gc.save(*mgr);
  std::cout << "set interface: " << (exists ? "updated" : "created") << " gre '"
            << tok.name() << "'\n";
}

bool InterfaceToken::showGreInterface(const InterfaceConfig &ic,
                                      ConfigurationManager *mgr) {
  std::vector<InterfaceConfig> v = {ic};
  auto gres = mgr->GetGreInterfaces(v);
  if (!gres.empty()) {
    SingleGreSummaryFormatter f;
    std::cout << f.format(gres[0]);
    return true;
  }
  return false;
}

std::string
InterfaceToken::showGreInterfaces(const std::vector<InterfaceConfig> &ifaces,
                                  ConfigurationManager *mgr) {
  GRETableFormatter f;
  return f.format(mgr->GetGreInterfaces(ifaces));
}
