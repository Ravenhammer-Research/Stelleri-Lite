#include "InterfaceToken.hpp"
#include "BridgeTableFormatter.hpp"
#include "InterfaceFlags.hpp"
#include "InterfaceTableFormatter.hpp"
#include "LaggTableFormatter.hpp"
#include "SingleInterfaceSummaryFormatter.hpp"
#include "TunnelTableFormatter.hpp"
#include "VLANTableFormatter.hpp"
#include "VirtualTableFormatter.hpp"
#include <iostream>
#include <sstream>

InterfaceToken::InterfaceToken(InterfaceType t, std::string name)
    : type_(t), name_(std::move(name)) {}

// textual reconstruction removed from InterfaceToken

std::vector<std::string> InterfaceToken::autoComplete(std::string_view) const {
  return {}; // suggestions could query ConfigurationManager; keep empty for
             // now
}

std::unique_ptr<Token> InterfaceToken::clone() const {
  return std::make_unique<InterfaceToken>(*this);
}

std::shared_ptr<InterfaceToken>
InterfaceToken::parseFromTokens(const std::vector<std::string> &tokens,
                                size_t start, size_t &next) {
  next = start + 1; // by default consume the 'interfaces' token
  if (start + 2 < tokens.size()) {
    // Support forms:
    //  - interfaces name <name>
    //  - interfaces <type> <name>
    //  - interfaces type <type> [name]
    const std::string &a = tokens[start + 1];
    const std::string &b = tokens[start + 2];

    // support `interfaces name <name>`
    if (a == "name") {
      std::string name = b;
      size_t nnext = start + 3;
      std::cerr << "[InterfaceToken] parseFromTokens: consumed 'name '" << b
                << " starting at " << start << " -> name='" << name
                << "' next=" << nnext << "\n";
      auto tok = std::make_shared<InterfaceToken>(InterfaceType::Unknown, name);
      // parse trailing options (vlan/lagg/etc.)
      size_t cur = nnext;
      while (cur < tokens.size()) {
        const std::string &kw = tokens[cur];
        if (kw == "vlan") {
          ++cur;
          std::optional<uint16_t> vid;
          std::optional<std::string> parent;
          while (cur < tokens.size()) {
            const std::string &k2 = tokens[cur];
            if (k2 == "id" && cur + 1 < tokens.size()) {
              vid = static_cast<uint16_t>(std::stoi(tokens[cur + 1]));
              cur += 2;
              continue;
            }
            if (k2 == "parent" && cur + 1 < tokens.size()) {
              parent = tokens[cur + 1];
              cur += 2;
              continue;
            }
            break;
          }
          if (vid && parent) {
            VLANConfig vc;
            vc.name = tok->name();
            vc.id = *vid;
            vc.parent = *parent;
            tok->vlan = std::move(vc);
          }
          continue;
        }
        if (kw == "lagg" || kw == "lag") {
          ++cur;
          LaggConfig lc;
          while (cur < tokens.size()) {
            const std::string &k2 = tokens[cur];
            if (k2 == "members" && cur + 1 < tokens.size()) {
              const std::string &m = tokens[cur + 1];
              size_t p = 0, len = m.size();
              while (p < len) {
                size_t q = m.find(',', p);
                if (q == std::string::npos) q = len;
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
              cur += 2;
              continue;
            }
            break;
          }
          if (!lc.members.empty())
            tok->lagg = std::move(lc);
          continue;
        }
        break;
      }
      next = cur;
      return tok;
    }
    InterfaceType itype = InterfaceType::Unknown;
    std::string name;

    if (a == "type") {
      // form: interfaces type <type> [name]
      const std::string &type = b;
      if (type == "ethernet")
        itype = InterfaceType::Ethernet;
      else if (type == "loopback")
        itype = InterfaceType::Loopback;
      else if (type == "ppp")
        itype = InterfaceType::PPP;
      else if (type == "bridge")
        itype = InterfaceType::Bridge;
      else if (type == "vlan")
        itype = InterfaceType::VLAN;
      else if (type == "lagg" || type == "lag")
        itype = InterfaceType::Lagg;
      else if (type == "tunnel")
        itype = InterfaceType::Tunnel;
      else if (type == "gif")
        itype = InterfaceType::Gif;
      else if (type == "tun")
        itype = InterfaceType::Tun;
      else if (type == "epair")
        itype = InterfaceType::Virtual;

      if (itype != InterfaceType::Unknown) {
        // optional name follows
        if (start + 3 < tokens.size())
          name = tokens[start + 3];
        next = start + (name.empty() ? 3 : 4);
        std::cerr << "[InterfaceToken] parseFromTokens: consumed 'type '" << b
                  << " starting at " << start
                  << " -> type=" << static_cast<int>(itype) << " name='" << name
                  << "' next=" << next << "\n";
        auto tok = std::make_shared<InterfaceToken>(itype, name);
        // Parse optional args after the name
        size_t cur = next;
        while (cur < tokens.size()) {
          const std::string &kw = tokens[cur];
          if (kw == "vlan") {
            // expect: vlan id <num> parent <ifname>
            ++cur;
            std::optional<uint16_t> vid;
            std::optional<std::string> parent;
            while (cur < tokens.size()) {
              const std::string &k2 = tokens[cur];
              if (k2 == "id" && cur + 1 < tokens.size()) {
                vid = static_cast<uint16_t>(std::stoi(tokens[cur + 1]));
                cur += 2;
                continue;
              }
              if (k2 == "parent" && cur + 1 < tokens.size()) {
                parent = tokens[cur + 1];
                cur += 2;
                continue;
              }
              break;
            }
            if (vid && parent) {
              VLANConfig vc;
              vc.name = tok->name();
              vc.id = *vid;
              vc.parent = *parent;
              tok->vlan = std::move(vc);
            }
            continue;
          }
          if (kw == "lagg" || kw == "lag") {
            ++cur;
            // expect: lagg members <if1,if2,...> [protocol <proto>]
            LaggConfig lc;
            while (cur < tokens.size()) {
              const std::string &k2 = tokens[cur];
              if (k2 == "members" && cur + 1 < tokens.size()) {
                // parse members token, allow comma-separated or multiple tokens
                const std::string &m = tokens[cur + 1];
                // split on commas
                size_t p = 0, len = m.size();
                while (p < len) {
                  size_t q = m.find(',', p);
                  if (q == std::string::npos) q = len;
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
                cur += 2;
                continue;
              }
              break;
            }
            if (!lc.members.empty())
              tok->lagg = std::move(lc);
            continue;
          }

          // Unknown keyword; stop parsing here
          break;
        }

        next = cur;
        return tok;
      }
    } else {
      // original form: interfaces <type> <name>
      const std::string &type = a;
      const std::string &nameTok = b;
      if (type == "ethernet")
        itype = InterfaceType::Ethernet;
      else if (type == "loopback")
        itype = InterfaceType::Loopback;
      else if (type == "ppp")
        itype = InterfaceType::PPP;
      else if (type == "bridge")
        itype = InterfaceType::Bridge;
      else if (type == "vlan")
        itype = InterfaceType::VLAN;
      else if (type == "lagg")
        itype = InterfaceType::Lagg;
      else if (type == "tunnel" || type == "gif")
        itype = InterfaceType::Tunnel;
      else if (type == "epair")
        itype = InterfaceType::Virtual;
      if (itype != InterfaceType::Unknown) {
        next = start + 3;
        std::cerr << "[InterfaceToken] parseFromTokens: consumed tokens '" 
                  << type << " ' '" << nameTok << "' starting at " << start
                  << " -> type=" << static_cast<int>(itype) << " name='"
                  << nameTok << "' next=" << next << "\n";
        auto tok = std::make_shared<InterfaceToken>(itype, nameTok);
        // parse additional args after nameTok
        size_t cur = next;
        while (cur < tokens.size()) {
          const std::string &kw = tokens[cur];
          if (kw == "vlan") {
            ++cur;
            std::optional<uint16_t> vid;
            std::optional<std::string> parent;
            while (cur < tokens.size()) {
              const std::string &k2 = tokens[cur];
              if (k2 == "id" && cur + 1 < tokens.size()) {
                vid = static_cast<uint16_t>(std::stoi(tokens[cur + 1]));
                cur += 2;
                continue;
              }
              if (k2 == "parent" && cur + 1 < tokens.size()) {
                parent = tokens[cur + 1];
                cur += 2;
                continue;
              }
              break;
            }
            if (vid && parent) {
              VLANConfig vc;
              vc.name = tok->name();
              vc.id = *vid;
              vc.parent = *parent;
              tok->vlan = std::move(vc);
            }
            continue;
          }
          if (kw == "lagg" || kw == "lag") {
            ++cur;
            LaggConfig lc;
            while (cur < tokens.size()) {
              const std::string &k2 = tokens[cur];
              if (k2 == "members" && cur + 1 < tokens.size()) {
                const std::string &m = tokens[cur + 1];
                size_t p = 0, len = m.size();
                while (p < len) {
                  size_t q = m.find(',', p);
                  if (q == std::string::npos) q = len;
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
                cur += 2;
                continue;
              }
              break;
            }
            if (!lc.members.empty())
              tok->lagg = std::move(lc);
            continue;
          }
          break;
        }

        next = cur;
        return tok;
      }
    }
  }
  // bare interfaces
  std::cerr << "[InterfaceToken] parseFromTokens: bare 'interfaces' at "
            << start << "\n";
  return std::make_shared<InterfaceToken>(InterfaceType::Unknown,
                                          std::string());
}
