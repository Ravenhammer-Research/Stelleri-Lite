/*
 * Copyright (c) 2026, Ravenhammer Research Inc.
 * All rights reserved.
 */

#include "ConfigurationManager.hpp"
#include "IPAddress.hpp"
#include "InterfaceToken.hpp"
#include "IpsecInterfaceConfig.hpp"
#include "IpsecTableFormatter.hpp"
#include "SingleIpsecSummaryFormatter.hpp"
#include <iostream>

class IpsecInterfaceToken : public InterfaceToken {
public:
  using InterfaceToken::InterfaceToken;
};

std::string InterfaceToken::toString(IpsecInterfaceConfig *cfg) {
  if (!cfg)
    return std::string();
  std::string s = InterfaceToken::toString(static_cast<InterfaceConfig *>(cfg));
  if (cfg->source)
    s += " source " + cfg->source->toString();
  if (cfg->destination)
    s += " destination " + cfg->destination->toString();
  if (cfg->tunnel_vrf)
    s += " tunnel-vrf " + std::to_string(*cfg->tunnel_vrf);
  if (cfg->reqid)
    s += " reqid " + std::to_string(*cfg->reqid);
  for (const auto &sa : cfg->security_associations) {
    s += " sa source " + sa.src + " destination " + sa.dst + " protocol " +
         sa.protocol + " spi " + std::to_string(sa.spi) + " algorithm " +
         sa.algorithm + " key " + sa.auth_key;
    if (sa.enc_algorithm)
      s += " enc-algorithm " + *sa.enc_algorithm;
    if (sa.enc_key)
      s += " enc-key " + *sa.enc_key;
  }
  for (const auto &sp : cfg->security_policies) {
    s += " sp direction " + sp.direction + " policy " + sp.policy;
    if (sp.reqid)
      s += " reqid " + std::to_string(*sp.reqid);
  }
  return s;
}

bool InterfaceToken::parseIpsecKeywords(std::shared_ptr<InterfaceToken> &tok,
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

  // sa source <src> destination <dst> protocol <ah|esp> spi <hex>
  //    algorithm <alg> key <hex> [enc-algorithm <alg> enc-key <hex>]
  if (kw == "sa") {
    ++cur;
    IpsecSA sa;
    while (cur < tokens.size()) {
      const std::string &sk = tokens[cur];
      if (sk == "source" && cur + 1 < tokens.size()) {
        sa.src = tokens[cur + 1];
        cur += 2;
        continue;
      }
      if (sk == "destination" && cur + 1 < tokens.size()) {
        sa.dst = tokens[cur + 1];
        cur += 2;
        continue;
      }
      if (sk == "protocol" && cur + 1 < tokens.size()) {
        sa.protocol = tokens[cur + 1];
        cur += 2;
        continue;
      }
      if (sk == "spi" && cur + 1 < tokens.size()) {
        sa.spi = static_cast<uint32_t>(std::stoul(tokens[cur + 1], nullptr, 0));
        cur += 2;
        continue;
      }
      if (sk == "algorithm" && cur + 1 < tokens.size()) {
        sa.algorithm = tokens[cur + 1];
        cur += 2;
        continue;
      }
      if (sk == "key" && cur + 1 < tokens.size()) {
        sa.auth_key = tokens[cur + 1];
        cur += 2;
        continue;
      }
      if (sk == "enc-algorithm" && cur + 1 < tokens.size()) {
        sa.enc_algorithm = tokens[cur + 1];
        cur += 2;
        continue;
      }
      if (sk == "enc-key" && cur + 1 < tokens.size()) {
        sa.enc_key = tokens[cur + 1];
        cur += 2;
        continue;
      }
      break;
    }
    tok->ipsec_sa = std::move(sa);
    return true;
  }

  // sp direction <in|out> policy <policy-string> [reqid <id>]
  if (kw == "sp") {
    ++cur;
    IpsecSP sp;
    while (cur < tokens.size()) {
      const std::string &sk = tokens[cur];
      if (sk == "direction" && cur + 1 < tokens.size()) {
        sp.direction = tokens[cur + 1];
        cur += 2;
        continue;
      }
      if (sk == "policy" && cur + 1 < tokens.size()) {
        sp.policy = tokens[cur + 1];
        cur += 2;
        continue;
      }
      if (sk == "reqid" && cur + 1 < tokens.size()) {
        sp.reqid =
            static_cast<uint32_t>(std::stoul(tokens[cur + 1], nullptr, 0));
        cur += 2;
        continue;
      }
      break;
    }
    tok->ipsec_sp = std::move(sp);
    return true;
  }

  if (kw == "reqid" && cur + 1 < tokens.size()) {
    tok->ipsec_reqid =
        static_cast<uint32_t>(std::stoul(tokens[cur + 1], nullptr, 0));
    cur += 2;
    return true;
  }

  return false;
}

std::vector<std::string>
InterfaceToken::ipsecCompletions(const std::string &prev) {
  if (prev.empty())
    return {"source", "destination", "tunnel-vrf", "sa", "sp", "reqid"};
  return {};
}

void InterfaceToken::setIpsecInterface(const InterfaceToken &tok,
                                       ConfigurationManager *mgr,
                                       InterfaceConfig &base, bool exists) {
  IpsecInterfaceConfig icfg(base);
  if (tok.source)
    icfg.source = IPAddress::fromString(*tok.source);
  if (tok.destination)
    icfg.destination = IPAddress::fromString(*tok.destination);
  if (tok.tunnel_vrf)
    icfg.tunnel_vrf = *tok.tunnel_vrf;
  if (tok.ipsec_sa)
    icfg.security_associations.push_back(*tok.ipsec_sa);
  if (tok.ipsec_sp)
    icfg.security_policies.push_back(*tok.ipsec_sp);
  if (tok.ipsec_reqid)
    icfg.reqid = tok.ipsec_reqid;
  icfg.save(*mgr);
  std::cout << "set interface: " << (exists ? "updated" : "created")
            << " ipsec '" << tok.name() << "'\n";
}

bool InterfaceToken::showIpsecInterface(const InterfaceConfig &ic,
                                        ConfigurationManager *mgr) {
  std::vector<InterfaceConfig> v = {ic};
  auto ipsecs = mgr->GetIpsecInterfaces(v);
  if (!ipsecs.empty()) {
    SingleIpsecSummaryFormatter f;
    std::cout << f.format(ipsecs[0]);
    return true;
  }
  return false;
}

std::string
InterfaceToken::showIpsecInterfaces(const std::vector<InterfaceConfig> &ifaces,
                                    ConfigurationManager *mgr) {
  IpsecTableFormatter f;
  return f.format(mgr->GetIpsecInterfaces(ifaces));
}
