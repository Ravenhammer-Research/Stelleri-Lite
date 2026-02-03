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
    // Support either: interfaces <type> <name>   OR   interfaces type <type>
    // [name]
    const std::string &a = tokens[start + 1];
    const std::string &b = tokens[start + 2];
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
      else if (type == "tunnel" || type == "gif")
        itype = InterfaceType::Tunnel;
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
        return std::make_shared<InterfaceToken>(itype, name);
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
                  << type << "' '" << nameTok << "' starting at " << start
                  << " -> type=" << static_cast<int>(itype) << " name='"
                  << nameTok << "' next=" << next << "\n";
        return std::make_shared<InterfaceToken>(itype, nameTok);
      }
    }
  }
  // bare interfaces
  std::cerr << "[InterfaceToken] parseFromTokens: bare 'interfaces' at "
            << start << "\n";
  return std::make_shared<InterfaceToken>(InterfaceType::Unknown,
                                          std::string());
}
