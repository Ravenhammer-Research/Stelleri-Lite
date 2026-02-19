/*
 * Copyright (c) 2026, Ravenhammer Research Inc.
 * All rights reserved.
 */

#include "ConfigurationManager.hpp"
#include "GifInterfaceConfig.hpp"
#include "GifTableFormatter.hpp"
#include "IPAddress.hpp"
#include "InterfaceToken.hpp"
#include "SingleGifSummaryFormatter.hpp"
#include <iostream>

class GifInterfaceToken : public InterfaceToken {
public:
  using InterfaceToken::InterfaceToken;
};

std::string InterfaceToken::toString(GifInterfaceConfig *cfg) {
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

bool InterfaceToken::parseGifKeywords(std::shared_ptr<InterfaceToken> &tok,
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
InterfaceToken::gifCompletions(const std::string &prev) {
  if (prev.empty())
    return {"source", "destination", "tunnel-vrf"};
  return {};
}

void InterfaceToken::setGifInterface(const InterfaceToken &tok,
                                     ConfigurationManager *mgr,
                                     InterfaceConfig &base, bool exists) {
  GifInterfaceConfig gc(base);
  if (tok.source)
    gc.source = IPAddress::fromString(*tok.source);
  if (tok.destination)
    gc.destination = IPAddress::fromString(*tok.destination);
  if (tok.tunnel_vrf)
    gc.tunnel_vrf = *tok.tunnel_vrf;
  gc.save(*mgr);
  std::cout << "set interface: " << (exists ? "updated" : "created") << " gif '"
            << tok.name() << "'\n";
}

bool InterfaceToken::showGifInterface(const InterfaceConfig &ic,
                                      ConfigurationManager *mgr) {
  std::vector<InterfaceConfig> v = {ic};
  auto gifs = mgr->GetGifInterfaces(v);
  if (!gifs.empty()) {
    SingleGifSummaryFormatter f;
    std::cout << f.format(gifs[0]);
    return true;
  }
  return false;
}

std::string
InterfaceToken::showGifInterfaces(const std::vector<InterfaceConfig> &ifaces,
                                  ConfigurationManager *mgr) {
  GifTableFormatter f;
  return f.format(mgr->GetGifInterfaces(ifaces));
}