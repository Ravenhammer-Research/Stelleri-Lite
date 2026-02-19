/*
 * Copyright (c) 2026, Ravenhammer Research Inc.
 * All rights reserved.
 */

#include "ConfigurationManager.hpp"
#include "InterfaceToken.hpp"
#include "SingleWlanSummaryFormatter.hpp"
#include "WlanAuthMode.hpp"
#include "WlanInterfaceConfig.hpp"
#include "WlanTableFormatter.hpp"
#include <iostream>

class WlanInterfaceToken : public InterfaceToken {
public:
  using InterfaceToken::InterfaceToken;
};

std::string InterfaceToken::toString(WlanInterfaceConfig *cfg) {
  if (!cfg)
    return std::string();
  std::string s = InterfaceToken::toString(static_cast<InterfaceConfig *>(cfg));
  if (cfg->ssid)
    s += " ssid " + *cfg->ssid;
  if (cfg->channel)
    s += " channel " + std::to_string(*cfg->channel);
  if (cfg->parent)
    s += " parent " + *cfg->parent;
  if (cfg->authmode) {
    switch (*cfg->authmode) {
    case WlanAuthMode::OPEN:
      s += " authmode open";
      break;
    case WlanAuthMode::SHARED:
      s += " authmode shared";
      break;
    case WlanAuthMode::WPA:
      s += " authmode wpa";
      break;
    default:
      break;
    }
  }
  return s;
}

bool InterfaceToken::parseWlanKeywords(std::shared_ptr<InterfaceToken> &tok,
                                       const std::vector<std::string> &tokens,
                                       size_t &cur) {
  const std::string &kw = tokens[cur];

  if (kw == "ssid" && cur + 1 < tokens.size()) {
    if (!tok->wlan)
      tok->wlan.emplace(InterfaceConfig{});
    tok->wlan->ssid = tokens[cur + 1];
    cur += 2;
    return true;
  }
  if (kw == "channel" && cur + 1 < tokens.size()) {
    if (!tok->wlan)
      tok->wlan.emplace(InterfaceConfig{});
    tok->wlan->channel = std::stoi(tokens[cur + 1]);
    cur += 2;
    return true;
  }
  if (kw == "parent" && cur + 1 < tokens.size()) {
    if (!tok->wlan)
      tok->wlan.emplace(InterfaceConfig{});
    tok->wlan->parent = tokens[cur + 1];
    cur += 2;
    return true;
  }
  if (kw == "authmode" && cur + 1 < tokens.size()) {
    if (!tok->wlan)
      tok->wlan.emplace(InterfaceConfig{});
    const std::string &mode = tokens[cur + 1];
    if (mode == "open")
      tok->wlan->authmode = WlanAuthMode::OPEN;
    else if (mode == "shared")
      tok->wlan->authmode = WlanAuthMode::SHARED;
    else if (mode == "wpa" || mode == "wpa2")
      tok->wlan->authmode = WlanAuthMode::WPA;
    cur += 2;
    return true;
  }
  return false;
}

std::vector<std::string>
InterfaceToken::wlanCompletions(const std::string &prev) {
  if (prev.empty())
    return {"ssid", "channel", "parent", "authmode"};
  if (prev == "authmode")
    return {"open", "shared", "wpa", "wpa2"};
  return {};
}

void InterfaceToken::setWlanInterface(const InterfaceToken &tok,
                                      ConfigurationManager *mgr,
                                      InterfaceConfig &base, bool exists) {
  WlanInterfaceConfig wc(base);
  if (tok.wlan) {
    if (tok.wlan->ssid)
      wc.ssid = tok.wlan->ssid;
    if (tok.wlan->channel)
      wc.channel = tok.wlan->channel;
    if (tok.wlan->parent)
      wc.parent = tok.wlan->parent;
    if (tok.wlan->authmode)
      wc.authmode = tok.wlan->authmode;
  }
  wc.save(*mgr);
  std::cout << "set interface: " << (exists ? "updated" : "created")
            << " wlan '" << tok.name() << "'\n";
}

bool InterfaceToken::showWlanInterface(const InterfaceConfig &ic,
                                       ConfigurationManager *mgr) {
  std::vector<InterfaceConfig> v = {ic};
  auto wlans = mgr->GetWlanInterfaces(v);
  if (!wlans.empty()) {
    SingleWlanSummaryFormatter f;
    std::cout << f.format(wlans[0]);
    return true;
  }
  return false;
}

std::string
InterfaceToken::showWlanInterfaces(const std::vector<InterfaceConfig> &ifaces,
                                   ConfigurationManager *mgr) {
  WlanTableFormatter f;
  return f.format(mgr->GetWlanInterfaces(ifaces));
}
