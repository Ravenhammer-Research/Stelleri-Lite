#include "RouteToken.hpp"

RouteToken::RouteToken(std::string prefix) : prefix_(std::move(prefix)) {}

// textual reconstruction removed from RouteToken

std::vector<std::string> RouteToken::autoComplete(std::string_view) const {
  return {"interface", "next-hop", "blackhole", "reject", "vrf"};
}

std::unique_ptr<Token> RouteToken::clone() const {
  auto r = std::make_unique<RouteToken>(prefix_);
  if (nexthop)
    r->nexthop = nexthop->clone();
  if (interface)
    r->interface = std::make_unique<InterfaceToken>(*interface);
  if (vrf)
    r->vrf = std::make_unique<VRFToken>(*vrf);
  r->blackhole = blackhole;
  r->reject = reject;
  return r;
}

void RouteToken::debugOutput(std::ostream &os) const {
  os << "[parser] parsed route: prefix='" << prefix_ << "'";
  if (nexthop)
    os << " nexthop='" << nexthop->toString() << "'";
  if (vrf)
    os << " vrf='" << vrf->name() << "'";
  if (interface)
    os << " interface='" << interface->name() << "'";
  if (blackhole)
    os << " blackhole=true";
  if (reject)
    os << " reject=true";
  os << '\n';
}

size_t RouteToken::parseOptions(const std::vector<std::string> &tokens,
                                size_t start) {
  size_t j = start;

  // Determine whether the provided `prefix_` is actually a network prefix or
  // whether the next token(s) are option keywords. If `prefix_` parses as a
  // network, treat it as the prefix and begin option parsing after it. If
  // `prefix_` does not parse as a network (for example it equals "vrf"),
  // clear it and treat the next tokens as options.
  if (!prefix_.empty()) {
    if (IPNetwork::fromString(prefix_)) {
      // prefix_ looks like a network; skip the token at `start` (caller
      // passed the prefix as tokens[start]).
      if (start < tokens.size() && tokens[start] == prefix_)
        j = start + 1;
      else
        j = start + 1; // still advance past the presumed prefix
    } else {
      // prefix_ is not a network (likely an option keyword); clear it and
      // start parsing options from `start`.
      prefix_.clear();
      j = start;
    }
  }
  while (j < tokens.size()) {
    const auto &opt = tokens[j];
    if ((opt == "next-hop" || opt == "nexthop") && j + 1 < tokens.size()) {
      const std::string nh = tokens[j + 1];
      // support shorthand: "nexthop reject" or "nexthop blackhole"
      if (nh == "reject") {
        reject = true;
        j += 2;
        continue;
      }
      if (nh == "blackhole") {
        blackhole = true;
        j += 2;
        continue;
      }
      std::unique_ptr<IPAddress> addr = IPAddress::fromString(nh);
      if (!addr) {
        auto net = IPNetwork::fromString(nh);
        if (net)
          addr = net->address();
      }
      nexthop = std::move(addr);
      j += 2;
      continue;
    }
    if (opt == "gw" && j + 1 < tokens.size()) {
      const std::string nh = tokens[j + 1];
      std::unique_ptr<IPAddress> addr = IPAddress::fromString(nh);
      if (!addr) {
        auto net = IPNetwork::fromString(nh);
        if (net)
          addr = net->address();
      }
      nexthop = std::move(addr);
      j += 2;
      continue;
    }
    if (opt == "dest" && j + 1 < tokens.size()) {
      // allow explicit 'dest <prefix>' syntax
      prefix_ = tokens[j + 1];
      j += 2;
      continue;
    }
    if (opt == "vrf" && j + 1 < tokens.size()) {
      vrf = std::make_unique<VRFToken>(tokens[j + 1]);
      j += 2;
      continue;
    }
    if (opt == "interface" && j + 1 < tokens.size()) {
      interface = std::make_unique<InterfaceToken>(InterfaceType::Unknown,
                                                   tokens[j + 1]);
      j += 2;
      continue;
    }
    if (opt == "blackhole") {
      blackhole = true;
      // blackhole represented by boolean only
      ++j;
      continue;
    }
    if (opt == "reject") {
      reject = true;
      // reject represented by boolean only
      ++j;
      continue;
    }
    break;
  }
  return j;
}
