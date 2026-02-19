/*
 * Copyright (c) 2026, Ravenhammer Research Inc.
 * All rights reserved.
 */

#include "ConfigurationManager.hpp"
#include "InterfaceToken.hpp"
#include "LaggInterfaceConfig.hpp"
#include "LaggProtocol.hpp"
#include "LaggTableFormatter.hpp"
#include "SingleLaggSummaryFormatter.hpp"
#include <iostream>

class LaggInterfaceToken : public InterfaceToken {
public:
  using InterfaceToken::InterfaceToken;
};

std::string InterfaceToken::toString(LaggInterfaceConfig *cfg) {
  if (!cfg)
    return std::string();
  std::string s = InterfaceToken::toString(static_cast<InterfaceConfig *>(cfg));
  for (const auto &m : cfg->members)
    s += " member " + m;
  switch (cfg->protocol) {
  case LaggProtocol::LACP:
    s += " protocol lacp";
    break;
  case LaggProtocol::FAILOVER:
    s += " protocol failover";
    break;
  case LaggProtocol::LOADBALANCE:
    s += " protocol loadbalance";
    break;
  case LaggProtocol::ROUNDROBIN:
    s += " protocol roundrobin";
    break;
  case LaggProtocol::BROADCAST:
    s += " protocol broadcast";
    break;
  case LaggProtocol::NONE:
    break;
  }
  return s;
}

bool InterfaceToken::parseLaggKeywords(std::shared_ptr<InterfaceToken> &tok,
                                       const std::vector<std::string> &tokens,
                                       size_t &cur) {
  const std::string &kw = tokens[cur];

  if (kw == "lagg" || kw == "lag") {
    ++cur;
    LaggInterfaceConfig lc;
    while (cur < tokens.size()) {
      const std::string &k2 = tokens[cur];
      if (k2 == "members" && cur + 1 < tokens.size()) {
        const std::string &m = tokens[cur + 1];
        size_t p = 0, len = m.size();
        while (p < len) {
          size_t q = m.find(',', p);
          if (q == std::string::npos)
            q = len;
          lc.members.emplace_back(m.substr(p, q - p));
          p = q + 1;
        }
        cur += 2;
        continue;
      }
      if (k2 == "protocol" && cur + 1 < tokens.size()) {
        const std::string &proto = tokens[cur + 1];
        if (proto == "lacp")
          lc.protocol = LaggProtocol::LACP;
        else if (proto == "failover")
          lc.protocol = LaggProtocol::FAILOVER;
        else if (proto == "loadbalance")
          lc.protocol = LaggProtocol::LOADBALANCE;
        else if (proto == "roundrobin")
          lc.protocol = LaggProtocol::ROUNDROBIN;
        else if (proto == "broadcast")
          lc.protocol = LaggProtocol::BROADCAST;
        else if (proto == "none")
          lc.protocol = LaggProtocol::NONE;
        cur += 2;
        continue;
      }
      break;
    }
    if (!lc.members.empty()) {
      tok->lagg.emplace();
      tok->lagg->members = std::move(lc.members);
      tok->lagg->protocol = lc.protocol;
      tok->lagg->hash_policy = lc.hash_policy;
      tok->lagg->lacp_rate = lc.lacp_rate;
      tok->lagg->min_links = lc.min_links;
    }
    return true;
  }

  if (kw == "protocol" && cur + 1 < tokens.size()) {
    const std::string &proto = tokens[cur + 1];
    if (!tok->lagg)
      tok->lagg.emplace();
    if (proto == "lacp")
      tok->lagg->protocol = LaggProtocol::LACP;
    else if (proto == "failover")
      tok->lagg->protocol = LaggProtocol::FAILOVER;
    else if (proto == "loadbalance")
      tok->lagg->protocol = LaggProtocol::LOADBALANCE;
    else if (proto == "roundrobin")
      tok->lagg->protocol = LaggProtocol::ROUNDROBIN;
    else if (proto == "broadcast")
      tok->lagg->protocol = LaggProtocol::BROADCAST;
    else if (proto == "none")
      tok->lagg->protocol = LaggProtocol::NONE;
    cur += 2;
    return true;
  }

  return false;
}

std::vector<std::string>
InterfaceToken::laggCompletions(const std::string &prev) {
  if (prev.empty())
    return {"lagg", "members", "protocol"};
  if (prev == "protocol")
    return {"lacp",       "failover",  "loadbalance",
            "roundrobin", "broadcast", "none"};
  return {};
}

void InterfaceToken::setLaggInterface(const InterfaceToken &tok,
                                      ConfigurationManager *mgr,
                                      InterfaceConfig &base, bool exists) {
  if (!tok.lagg || tok.lagg->members.empty()) {
    std::cerr << "set interface: LAGG creation typically requires member "
                 "interfaces.\n"
              << "Usage: set interface name <lagg_name> lagg members "
                 "<if1,if2,...> [protocol <proto>]\n";
    return;
  }
  LaggInterfaceConfig lac(base, tok.lagg->protocol, tok.lagg->members,
                          tok.lagg->hash_policy, tok.lagg->lacp_rate,
                          tok.lagg->min_links);
  lac.save(*mgr);
  std::cout << "set interface: " << (exists ? "updated" : "created")
            << " lagg '" << tok.name() << "'\n";
}

bool InterfaceToken::showLaggInterface(const InterfaceConfig &ic,
                                       ConfigurationManager *mgr) {
  std::vector<InterfaceConfig> v = {ic};
  auto laggs = mgr->GetLaggInterfaces(v);
  if (!laggs.empty()) {
    SingleLaggSummaryFormatter f;
    std::cout << f.format(laggs[0]);
    return true;
  }
  return false;
}

std::string
InterfaceToken::showLaggInterfaces(const std::vector<InterfaceConfig> &ifaces,
                                   ConfigurationManager *mgr) {
  LaggTableFormatter f;
  return f.format(mgr->GetLaggInterfaces(ifaces));
}
