/*
 * Copyright (c) 2026, Ravenhammer Research Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "InterfaceToken.hpp"
#include "IPAddress.hpp"
#include "IPNetwork.hpp"
#include "InterfaceConfig.hpp"
#include "InterfaceFlags.hpp"
#include "InterfaceTableFormatter.hpp"
#include "InterfaceType.hpp"
#include "SingleInterfaceSummaryFormatter.hpp"
#include <iostream>
#include <netinet/in.h>
#include <unordered_set>

InterfaceToken::InterfaceToken(InterfaceType t, std::string name)
    : type_(t), name_(std::move(name)) {}

// ---------------------------------------------------------------------------
// dispatch — central InterfaceType → handler lookup table
// ---------------------------------------------------------------------------
const InterfaceTypeDispatch *InterfaceToken::dispatch(InterfaceType t) {
  struct Entry {
    InterfaceType type;
    InterfaceTypeDispatch info;
  };
  static const Entry table[] = {
      {InterfaceType::Bridge,
       {"bridge", "bridge", bridgeCompletions, parseBridgeKeywords,
        setBridgeInterface, showBridgeInterface, showBridgeInterfaces}},
      {InterfaceType::VLAN,
       {"vlan", "vlan", vlanCompletions, parseVlanKeywords, setVlanInterface,
        showVlanInterface, showVlanInterfaces}},
      {InterfaceType::Lagg,
       {"lagg", "lagg", laggCompletions, parseLaggKeywords, setLaggInterface,
        showLaggInterface, showLaggInterfaces}},
      {InterfaceType::Tunnel,
       {"tunnel", nullptr, tunCompletions, parseTunKeywords, setTunInterface,
        showTunInterface, showTunInterfaces}},
      {InterfaceType::Tun,
       {"tun", "tun", tunCompletions, parseTunKeywords, setTunInterface,
        showTunInterface, showTunInterfaces}},
      {InterfaceType::Gif,
       {"gif", "gif", gifCompletions, parseGifKeywords, setGifInterface,
        showGifInterface, showGifInterfaces}},
      {InterfaceType::GRE,
       {"gre", "gre", greCompletions, parseGreKeywords, setGreInterface,
        showGreInterface, showGreInterfaces}},
      {InterfaceType::VXLAN,
       {"vxlan", "vxlan", vxlanCompletions, parseVxlanKeywords,
        setVxlanInterface, showVxlanInterface, showVxlanInterfaces}},
      {InterfaceType::IPsec,
       {"ipsec", "ipsec", ipsecCompletions, parseIpsecKeywords,
        setIpsecInterface, showIpsecInterface, showIpsecInterfaces}},
      {InterfaceType::Ovpn,
       {"ovpn", "ovpn", ovpnCompletions, parseOvpnKeywords, setOvpnInterface,
        showOvpnInterface, showOvpnInterfaces}},
      {InterfaceType::Carp,
       {"carp", "carp", carpCompletions, parseCarpKeywords, setCarpInterface,
        showCarpInterface, showCarpInterfaces}},
      {InterfaceType::Wireless,
       {"wireless", nullptr, wlanCompletions, parseWlanKeywords,
        setWlanInterface, showWlanInterface, showWlanInterfaces}},
      {InterfaceType::WireGuard,
       {"wg", "wg", wireGuardCompletions, parseWireGuardKeywords,
        setWireGuardInterface, nullptr, showWireGuardInterfaces}},
      {InterfaceType::Tap,
       {"tap", "tap", tapCompletions, parseTapKeywords, setTapInterface,
        nullptr, showTapInterfaces}},
      {InterfaceType::SixToFour,
       {"stf", "stf", sixToFourCompletions, parseSixToFourKeywords,
        setSixToFourInterface, nullptr, showSixToFourInterfaces}},
      {InterfaceType::Pflog,
       {"pflog", "pflog", pflogCompletions, parsePflogKeywords,
        setPflogInterface, nullptr, showPflogInterfaces}},
      {InterfaceType::Pfsync,
       {"pfsync", "pfsync", pfsyncCompletions, parsePfsyncKeywords,
        setPfsyncInterface, nullptr, showPfsyncInterfaces}},
      {InterfaceType::Epair,
       {"epair", "epair", epairCompletions, parseEpairKeywords,
        setEpairInterface, showEpairInterface, showEpairInterfaces}},
      {InterfaceType::Loopback,
       {"loopback", "lo", loopbackCompletions, parseLoopbackKeywords,
        setLoopbackInterface, nullptr, showLoopbackInterfaces}},
  };
  for (const auto &e : table)
    if (e.type == t)
      return &e.info;
  return nullptr;
}

// ---------------------------------------------------------------------------
// Helpers: keyword sets for context-aware autocompletion
// ---------------------------------------------------------------------------
namespace {
  /// Keywords applicable to every interface type.
  std::vector<std::string> generalKeywords() {
    return {"inet",  "inet6", "address", "mtu",    "vrf",
            "group", "up",    "down",    "status", "description"};
  }

  /// Value suggestions for a keyword that expects a fixed set of values.
  std::vector<std::string> valuesForKeyword(const std::string &kw) {
    if (kw == "type") {
      return {"ethernet", "loopback", "bridge", "lagg",  "vlan",  "tunnel",
              "tun",      "gif",      "gre",    "vxlan", "ipsec", "epair",
              "virtual",  "wireless", "tap",    "ppp",   "stf",   "ovpn",
              "carp",     "pflog",    "pfsync", "wg"};
    }
    if (kw == "status") {
      return {"up", "down"};
    }
    return {};
  }

  /// Filter a candidate list by prefix match.
  std::vector<std::string>
  filterPrefix(const std::vector<std::string> &candidates,
               std::string_view partial) {
    std::vector<std::string> matches;
    for (const auto &c : candidates) {
      if (c.rfind(partial, 0) == 0)
        matches.push_back(c);
    }
    return matches;
  }

} // anonymous namespace

// ---------------------------------------------------------------------------
// autoComplete (stateless stub — satisfies Token pure virtual)
// The real logic lives in the context-aware overload below.
// ---------------------------------------------------------------------------
std::vector<std::string>
InterfaceToken::autoComplete(std::string_view /*partial*/) const {
  return {};
}

std::unique_ptr<Token> InterfaceToken::clone() const {
  return std::make_unique<InterfaceToken>(*this);
}

// ---------------------------------------------------------------------------
// autoComplete (context-aware — with manager and token history)
// ---------------------------------------------------------------------------
std::vector<std::string>
InterfaceToken::autoComplete(const std::vector<std::string> &tokens,
                             std::string_view partial,
                             ConfigurationManager *mgr) const {
  std::vector<std::string> matches;

  // If user typed `type` and is now completing the type value, suggest types
  if (expect_type_value_)
    return filterPrefix(valuesForKeyword("type"), partial);

  const std::string prev = tokens.empty() ? std::string() : tokens.back();

  // --- Value completions for keywords that expect a fixed set of values ---

  // After "type", suggest type names
  if (prev == "type") {
    return filterPrefix(valuesForKeyword("type"), partial);
  }

  // Type-specific value completions (stp, protocol, authmode, etc.)
  if (type_ != InterfaceType::Unknown) {
    auto vals = typeCompletions(type_, prev);
    if (!vals.empty())
      return filterPrefix(vals, partial);
  }

  // After "status", suggest up/down
  if (prev == "status") {
    return filterPrefix(valuesForKeyword("status"), partial);
  }

  // --- Value completions from live system state (require mgr) ---

  if (mgr) {
    // After "group", list all known interface groups
    if (prev == "group") {
      std::unordered_set<std::string> groups;
      auto ifs = mgr->GetInterfaces();
      for (const auto &i : ifs) {
        for (const auto &g : i.groups) {
          groups.insert(g);
        }
      }
      for (const auto &g : groups) {
        if (g.rfind(partial, 0) == 0)
          matches.push_back(g);
      }
      return matches;
    }

    // After "name", list all known interface names
    if (prev == "name") {
      auto ifs = mgr->GetInterfaces();
      for (const auto &i : ifs) {
        const std::string &iname = i.name;
        if (iname.rfind(partial, 0) == 0)
          matches.push_back(iname);
      }
      return matches;
    }

    // After "member" or "parent", suggest available interface names
    if (prev == "member" || prev == "parent") {
      auto ifs = mgr->GetInterfaces();
      for (const auto &i : ifs) {
        // Don't suggest the interface being configured as its own member/parent
        if (i.name == name_)
          continue;
        if (i.name.rfind(partial, 0) == 0)
          matches.push_back(i.name);
      }
      return matches;
    }

    // After "members" (lagg), suggest comma-separated interface names
    if (prev == "members") {
      auto ifs = mgr->GetInterfaces();
      // If partial already contains a comma, complete the last segment
      std::string prefix_part;
      std::string last_seg(partial);
      auto comma_pos = last_seg.rfind(',');
      if (comma_pos != std::string::npos) {
        prefix_part = last_seg.substr(0, comma_pos + 1);
        last_seg = last_seg.substr(comma_pos + 1);
      }
      for (const auto &i : ifs) {
        if (i.name == name_)
          continue;
        if (i.name.rfind(last_seg, 0) == 0)
          matches.push_back(prefix_part + i.name);
      }
      return matches;
    }
  } // mgr

  // If the previous token is a known interface name (i.e. we are past
  // "interface name <ifname>"), suggest type-specific and general keywords.
  if (tokens.size() >= 2 && tokens[tokens.size() - 2] == "name") {
    auto opts = generalKeywords();
    opts.push_back("type");
    opts.push_back("group");
    // If we can detect the type from the name, add type-specific keywords
    if (type_ != InterfaceType::Unknown) {
      auto extra = typeCompletions(type_, "");
      opts.insert(opts.end(), extra.begin(), extra.end());
    }
    return filterPrefix(opts, partial);
  }

  // If a type has been set (from earlier tokens), offer type-aware keywords
  if (type_ != InterfaceType::Unknown) {
    auto opts = generalKeywords();
    auto extra = typeCompletions(type_, "");
    opts.insert(opts.end(), extra.begin(), extra.end());
    return filterPrefix(opts, partial);
  }

  // Top-level: no name, no type, nothing set yet → primary keywords
  if (name_.empty() && type_ == InterfaceType::Unknown && !vrf && !mtu &&
      !status && !vlan && !lagg && !bridge && !tun && !gif && !ovpn && !ipsec &&
      !vxlan && !wlan && !gre && !carp) {
    return filterPrefix({"name", "group", "type"}, partial);
  }

  // Name set but no attributes yet → narrow choices
  if (!name_.empty() && type_ == InterfaceType::Unknown) {
    auto opts = generalKeywords();
    opts.push_back("type");
    opts.push_back("group");
    return filterPrefix(opts, partial);
  }

  // Fallback: only suggest structural keywords that help narrow context.
  // Type-specific keywords are never offered until a type is known.
  return filterPrefix({"name", "type", "group"}, partial);
}

// ---------------------------------------------------------------------------
// Static renderers for interface configs
// ---------------------------------------------------------------------------
std::string InterfaceToken::toString(InterfaceConfig *cfg) {
  if (!cfg)
    return std::string();
  std::string result = "interface name " + cfg->name;
  if (auto *d = dispatch(cfg->type))
    result += std::string(" type ") + d->typeName;
  if (cfg->vrf)
    result += " vrf " + std::to_string(cfg->vrf->table);
  if (cfg->mtu)
    result += " mtu " + std::to_string(*cfg->mtu);
  if (cfg->address)
    result += " address " + cfg->address->toString();
  if (!cfg->groups.empty()) {
    // Skip the default group that FreeBSD assigns automatically.
    auto *dg = dispatch(cfg->type);
    const char *defaultGrp = (dg && dg->defaultGroup) ? dg->defaultGroup : "";
    for (const auto &g : cfg->groups) {
      if (g == "all" || g == defaultGrp)
        continue;
      result += " group " + g;
    }
  }
  return result;
}

// ---------------------------------------------------------------------------
// typeCompletions — dispatch to per-type completion providers
// ---------------------------------------------------------------------------
std::vector<std::string>
InterfaceToken::typeCompletions(InterfaceType t, const std::string &prev) {
  if (auto *d = dispatch(t); d && d->completions)
    return d->completions(prev);
  return {};
}

// ---------------------------------------------------------------------------
// parseKeywords — general keywords + type-based dispatch
// ---------------------------------------------------------------------------
void InterfaceToken::parseKeywords(std::shared_ptr<InterfaceToken> &tok,
                                   const std::vector<std::string> &tokens,
                                   size_t &cur) {
  while (cur < tokens.size()) {
    const std::string &kw = tokens[cur];

    // --- General keywords (all types) ---
    if (kw == "inet" || kw == "inet6") {
      tok->address_family = (kw == "inet") ? AF_INET : AF_INET6;
      if (cur + 1 < tokens.size() && tokens[cur + 1] == "address" &&
          cur + 2 < tokens.size()) {
        tok->address = tokens[cur + 2];
        cur += 3;
        continue;
      }
      ++cur;
      continue;
    }
    if (kw == "group" && cur + 1 < tokens.size()) {
      tok->group = tokens[cur + 1];
      cur += 2;
      continue;
    }
    if (kw == "mtu" && cur + 1 < tokens.size()) {
      tok->mtu = std::stoi(tokens[cur + 1]);
      cur += 2;
      continue;
    }
    if ((kw == "fib" || kw == "vrf") && cur + 1 < tokens.size()) {
      tok->vrf = std::stoi(tokens[cur + 1]);
      cur += 2;
      continue;
    }
    if (kw == "up") {
      tok->status = true;
      ++cur;
      continue;
    }
    if (kw == "down") {
      tok->status = false;
      ++cur;
      continue;
    }
    if (kw == "status" && cur + 1 < tokens.size()) {
      const std::string &val = tokens[cur + 1];
      if (val == "up")
        tok->status = true;
      else if (val == "down")
        tok->status = false;
      cur += 2;
      continue;
    }
    if (kw == "description" && cur + 1 < tokens.size()) {
      tok->description = tokens[cur + 1];
      cur += 2;
      continue;
    }

    // --- Type-specific keywords ---
    bool consumed = false;
    if (auto *d = dispatch(tok->type()); d && d->parseKeywords)
      consumed = d->parseKeywords(tok, tokens, cur);
    if (consumed)
      continue;

    // Unknown keyword; stop parsing
    break;
  }
}

// ---------------------------------------------------------------------------
// parseFromTokens — entry point
// ---------------------------------------------------------------------------
std::shared_ptr<InterfaceToken>
InterfaceToken::parseFromTokens(const std::vector<std::string> &tokens,
                                size_t start, size_t &next) {
  next = start + 1; // by default consume the 'interfaces' token
  if (start + 1 < tokens.size()) {
    // There is at least one token after 'interfaces'
    const std::string &a = tokens[start + 1];
    const std::string b =
        (start + 2 < tokens.size()) ? tokens[start + 2] : std::string();

    // support `interfaces group <group>`
    if (a == "group") {
      if (!b.empty()) {
        std::string grp = b;
        size_t nnext = start + 3;
        auto tok = std::make_shared<InterfaceToken>(InterfaceType::Unknown,
                                                    std::string());
        tok->group = grp;
        next = nnext;
        return tok;
      }
    }

    // support `interfaces name <name>`
    if (a == "name") {
      if (!b.empty()) {
        std::string name = b;
        auto tok =
            std::make_shared<InterfaceToken>(InterfaceType::Unknown, name);
        size_t cur = start + 3;
        parseKeywords(tok, tokens, cur);
        next = cur;
        return tok;
      }
    }

    InterfaceType itype = InterfaceType::Unknown;
    std::string name;

    if (a == "type") {
      // form: interfaces type <type> [name]
      if (start + 2 >= tokens.size()) {
        // user typed 'type' but no value yet
        auto tok = std::make_shared<InterfaceToken>(InterfaceType::Unknown,
                                                    std::string());
        tok->expect_type_value_ = true;
        next = start + 2;
        return tok;
      }

      itype = interfaceTypeFromString(b);

      if (itype != InterfaceType::Unknown) {
        // optional name follows; allow `name <name>` form as well
        if (start + 3 < tokens.size()) {
          if (tokens[start + 3] == "name" && start + 4 < tokens.size())
            name = tokens[start + 4];
          else
            name = tokens[start + 3];
        }
        size_t cur =
            start + (name.empty() ? 3 : (tokens[start + 3] == "name" ? 5 : 4));

        auto tok = std::make_shared<InterfaceToken>(itype, name);
        parseKeywords(tok, tokens, cur);
        next = cur;
        return tok;
      }
    } else {
      // original form: interfaces <type> <name>
      itype = interfaceTypeFromString(a);
      if (itype != InterfaceType::Unknown && !b.empty()) {
        auto tok = std::make_shared<InterfaceToken>(itype, b);
        size_t cur = start + 3;
        parseKeywords(tok, tokens, cur);
        next = cur;
        return tok;
      }
    }
  }
  // bare interfaces
  return std::make_shared<InterfaceToken>(InterfaceType::Unknown,
                                          std::string());
}

// ---------------------------------------------------------------------------
// executeSet — common base setup + type dispatch
// ---------------------------------------------------------------------------
void InterfaceToken::executeSet(ConfigurationManager *mgr) const {
  if (name_.empty()) {
    std::cerr << "set interface: missing interface name\n";
    return;
  }

  try {
    bool exists = InterfaceConfig::exists(*mgr, name_);
    auto ifopt = (exists && mgr) ? mgr->GetInterface(name_)
                                 : std::optional<InterfaceConfig>{};
    InterfaceConfig base = ifopt ? *ifopt : InterfaceConfig();
    if (!ifopt)
      base.name = name_;

    if (vrf) {
      if (!base.vrf)
        base.vrf = std::make_unique<VRFConfig>(*vrf);
      else
        base.vrf->table = *vrf;
    }

    InterfaceType effectiveType = InterfaceType::Unknown;
    if (type_ != InterfaceType::Unknown)
      effectiveType = type_;
    else if (base.type != InterfaceType::Unknown)
      effectiveType = base.type;

    if (address) {
      auto net = IPNetwork::fromString(*address);
      if (net) {
        if (!base.address)
          base.address = std::move(net);
        else
          base.aliases.emplace_back(net->clone());
      } else {
        std::cerr << "set interface: invalid address '" << *address << "'\n";
      }
    }

    if (group) {
      bool has_group = false;
      for (const auto &g : base.groups) {
        if (g == *group) {
          has_group = true;
          break;
        }
      }
      if (!has_group)
        base.groups.push_back(*group);
    }

    if (mtu)
      base.mtu = *mtu;
    else
      base.mtu.reset();

    if (status) {
      if (base.flags) {
        if (*status)
          *base.flags |= flagBit(InterfaceFlag::UP);
        else
          *base.flags &= ~flagBit(InterfaceFlag::UP);
      } else {
        base.flags = *status ? flagBit(InterfaceFlag::UP) : 0u;
      }
    }

    if (description)
      base.description = *description;

    if (auto *d = dispatch(effectiveType); d && d->setInterface) {
      d->setInterface(*this, mgr, base, exists);
      return;
    }

    // No specific type matched — alias or generic update
    if (address && exists) {
      auto net = IPNetwork::fromString(*address);
      if (!net) {
        std::cerr << "set interface: invalid address '" << *address << "'\n";
        return;
      }
      base.aliases.emplace_back(net->clone());
      base.save(*mgr);
      std::cout << "set interface: added alias '" << *address << "' to '"
                << name_ << "'\n";
      return;
    }

    base.save(*mgr);
    std::cout << "set interface: " << (exists ? "updated" : "created")
              << " interface '" << name_ << "'\n";
  } catch (const std::exception &e) {
    std::cerr << "set interface: failed to create/update '" << name_
              << "': " << e.what() << "\n";
  }
}

// ---------------------------------------------------------------------------
// executeShow — collection + single-interface dispatch + multi-interface table
// ---------------------------------------------------------------------------
void InterfaceToken::executeShow(ConfigurationManager *mgr) const {
  if (!mgr) {
    std::cout << "No ConfigurationManager provided\n";
    return;
  }
  std::vector<InterfaceConfig> interfaces;
  if (!name_.empty()) {
    auto ifopt = mgr->GetInterface(name_);
    if (ifopt)
      interfaces.push_back(std::move(*ifopt));
  } else if (type_ != InterfaceType::Unknown) {
    auto allIfaces = mgr->GetInterfaces();
    for (auto &iface : allIfaces) {
      if (group) {
        bool has = false;
        for (const auto &g : iface.groups) {
          if (g == *group) {
            has = true;
            break;
          }
        }
        if (!has)
          continue;
      }
      if (iface.matchesType(type_))
        interfaces.push_back(std::move(iface));
    }
  } else {
    if (group)
      interfaces = mgr->GetInterfacesByGroup(std::nullopt, *group);
    else
      interfaces = mgr->GetInterfaces();
  }

  if (group && type_ == InterfaceType::Unknown) {
    InterfaceTableFormatter formatter;
    std::cout << formatter.format(interfaces);
    return;
  }

  if (interfaces.size() == 1 && !name_.empty()) {
    const auto &ic = interfaces[0];
    bool handled = false;
    if (auto *d = dispatch(ic.type); d && d->showInterface)
      handled = d->showInterface(ic, mgr);
    if (handled)
      return;
    SingleInterfaceSummaryFormatter f;
    std::cout << f.format(ic);
    return;
  }

  // Multi-interface: use type-specific table formatter when type is known
  if (auto *d = dispatch(type_); d && d->showInterfaces) {
    std::cout << d->showInterfaces(interfaces, mgr);
    return;
  }

  std::cout << InterfaceConfig::formatInterfaces(interfaces, mgr);
}

// ---------------------------------------------------------------------------
// executeDelete — group/address/destroy
// ---------------------------------------------------------------------------
void InterfaceToken::executeDelete(ConfigurationManager *mgr) const {
  if (name_.empty()) {
    std::cerr << "delete interface: missing interface name\n";
    return;
  }

  if (!InterfaceConfig::exists(*mgr, name_)) {
    std::cerr << "delete interface: interface '" << name_ << "' not found\n";
    return;
  }

  InterfaceConfig ic;
  ic.name = name_;
  try {
    if (group) {
      mgr->RemoveInterfaceGroup(name_, *group);
      std::cout << "delete interface: removed group '" << *group << "' from '"
                << name_ << "'\n";
      return;
    }

    if (address || address_family) {
      std::vector<std::string> to_remove;
      if (address) {
        to_remove.push_back(*address);
      } else if (address_family && *address_family == AF_INET) {
        to_remove = mgr->GetInterfaceAddresses(name_, AF_INET);
      } else if (address_family && *address_family == AF_INET6) {
        to_remove = mgr->GetInterfaceAddresses(name_, AF_INET6);
      }

      for (const auto &a : to_remove) {
        try {
          ic.removeAddress(*mgr, a);
          std::cout << "delete interface: removed address '" << a << "' from '"
                    << name_ << "'\n";
        } catch (const std::exception &e) {
          std::cerr << "delete interface: failed to remove address '" << a
                    << "': " << e.what() << "\n";
        }
      }
      return;
    }

    ic.destroy(*mgr);
    std::cout << "delete interface: removed '" << name_ << "'\n";
  } catch (const std::exception &e) {
    std::cerr << "delete interface: failed to remove '" << name_
              << "': " << e.what() << "\n";
  }
}
