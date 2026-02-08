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

#include "RouteToken.hpp"

RouteToken::RouteToken(std::string prefix) : prefix_(std::move(prefix)) {}

std::string RouteToken::toString() const {
  std::string result = "route protocol static dest " + prefix_;

  if (blackhole) {
    result += " nexthop blackhole";
  } else if (reject) {
    result += " nexthop reject";
  } else if (nexthop) {
    result += " nexthop " + nexthop->toString();
    if (interface) {
      result += " interface " + interface->name();
    }
  } else if (interface) {
    // No nexthop IP, only interface - use nexthop-interface
    result += " nexthop-interface " + interface->name();
  }

  if (vrf) {
    result += " vrf " + std::to_string(vrf->table());
  }

  if (next_) {
    result += " " + next_->toString();
  }
  return result;
}

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
    os << " vrf='" << vrf->table() << "'";
  if (interface)
    os << " interface='" << interface->name() << "'";
  if (blackhole)
    os << " blackhole=true";
  if (reject)
    os << " reject=true";
  os << '\n';
}

std::shared_ptr<RouteToken>
RouteToken::parseFromTokens(const std::vector<std::string> &tokens,
                            size_t start, size_t &next) {
  next = start + 1; // consume the 'route' or 'routes' token

  std::string prefix;
  size_t j = next;

  // Check if there's a potential prefix at the next position
  if (j < tokens.size()) {
    const std::string &candidate = tokens[j];
    // If it parses as a network, use it as the prefix
    if (IPNetwork::fromString(candidate)) {
      prefix = candidate;
      ++j;
    }
  }

  auto tok = std::make_shared<RouteToken>(prefix);

  while (j < tokens.size()) {
    const auto &opt = tokens[j];
    if ((opt == "next-hop" || opt == "nexthop") && j + 1 < tokens.size()) {
      const std::string nh = tokens[j + 1];
      // support shorthand: "nexthop reject" or "nexthop blackhole"
      if (nh == "reject") {
        tok->reject = true;
        j += 2;
        continue;
      }
      if (nh == "blackhole") {
        tok->blackhole = true;
        j += 2;
        continue;
      }
      std::unique_ptr<IPAddress> addr = IPAddress::fromString(nh);
      if (!addr) {
        auto net = IPNetwork::fromString(nh);
        if (net)
          addr = net->address();
      }
      tok->nexthop = std::move(addr);
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
      tok->nexthop = std::move(addr);
      j += 2;
      continue;
    }
    if (opt == "dest" && j + 1 < tokens.size()) {
      // allow explicit 'dest <prefix>' syntax
      tok->prefix_ = tokens[j + 1];
      j += 2;
      continue;
    }
    if (opt == "vrf" && j + 1 < tokens.size()) {
      tok->vrf = std::make_unique<VRFToken>(std::stoi(tokens[j + 1]));
      j += 2;
      continue;
    }
    if (opt == "interface" && j + 1 < tokens.size()) {
      tok->interface = std::make_unique<InterfaceToken>(InterfaceType::Unknown,
                                                        tokens[j + 1]);
      j += 2;
      continue;
    }
    if (opt == "blackhole") {
      tok->blackhole = true;
      // blackhole represented by boolean only
      ++j;
      continue;
    }
    if (opt == "reject") {
      tok->reject = true;
      // reject represented by boolean only
      ++j;
      continue;
    }
    break;
  }

  next = j;
  return tok;
}
