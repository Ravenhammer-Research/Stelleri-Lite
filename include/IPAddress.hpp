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

#pragma once

#include "AddressFamily.hpp"
#include <arpa/inet.h>
#include <cstdint>
#include <cstring>
#include <memory>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>

// Polymorphic IP address base type
class IPAddress {
public:
  virtual ~IPAddress() = default;
  virtual AddressFamily family() const = 0;
  virtual std::string toString() const = 0;
  virtual std::unique_ptr<IPAddress> clone() const = 0;

  // Parse textual address (IPv4 or IPv6)
  static std::unique_ptr<IPAddress> fromString(const std::string &s);

  // Create a subnet mask IPAddress from a CIDR prefix length for the
  // specified address family (IPv4 or IPv6). Returns nullptr on invalid
  // input (e.g., cidr > 32 for IPv4 or >128 for IPv6).
  static std::unique_ptr<IPAddress> maskFromCIDR(AddressFamily fam,
                                                 uint8_t cidr);
};

// IPv4 concrete implementation
class IPv4Address : public IPAddress {
  uint32_t v_ = 0;

public:
  IPv4Address() = default;
  explicit IPv4Address(uint32_t v) : v_(v) {}
  explicit IPv4Address(const std::string &s) {
    struct in_addr a4;
    if (inet_pton(AF_INET, s.c_str(), &a4) == 1) {
      v_ = ntohl(a4.s_addr);
    } else {
      v_ = 0;
    }
  }

  AddressFamily family() const override { return AddressFamily::IPv4; }
  uint32_t value() const { return v_; }

  std::string toString() const override {
    struct in_addr a4;
    a4.s_addr = htonl(v_);
    char buf[INET_ADDRSTRLEN] = {0};
    if (inet_ntop(AF_INET, &a4, buf, sizeof(buf)) == nullptr)
      return std::string();
    return std::string(buf);
  }

  std::unique_ptr<IPAddress> clone() const override {
    return std::make_unique<IPv4Address>(v_);
  }
};

// IPv6 concrete implementation
class IPv6Address : public IPAddress {
  unsigned __int128 v_ = 0;

public:
  IPv6Address() = default;
  explicit IPv6Address(unsigned __int128 v) : v_(v) {}
  explicit IPv6Address(const std::string &s) {
    struct in6_addr a6;
    if (inet_pton(AF_INET6, s.c_str(), &a6) == 1) {
      unsigned __int128 v = 0;
      for (int i = 0; i < 16; ++i) {
        v <<= 8;
        v |= static_cast<unsigned char>(a6.s6_addr[i]);
      }
      v_ = v;
    } else {
      v_ = 0;
    }
  }

  AddressFamily family() const override { return AddressFamily::IPv6; }
  unsigned __int128 value() const { return v_; }

  std::string toString() const override {
    struct in6_addr a6;
    unsigned __int128 v = v_;
    for (int i = 15; i >= 0; --i) {
      a6.s6_addr[i] = static_cast<uint8_t>(v & 0xFF);
      v >>= 8;
    }
    char buf[INET6_ADDRSTRLEN] = {0};
    if (inet_ntop(AF_INET6, &a6, buf, sizeof(buf)) == nullptr)
      return std::string();
    return std::string(buf);
  }

  std::unique_ptr<IPAddress> clone() const override {
    return std::make_unique<IPv6Address>(v_);
  }
};

// Factory: parse an address string (IPv4 or IPv6)
inline std::unique_ptr<IPAddress> IPAddress::fromString(const std::string &s) {
  // Try IPv4
  struct in_addr a4;
  if (inet_pton(AF_INET, s.c_str(), &a4) == 1) {
    uint32_t v = ntohl(a4.s_addr);
    return std::make_unique<IPv4Address>(v);
  }
  // Try IPv6
  struct in6_addr a6;
  if (inet_pton(AF_INET6, s.c_str(), &a6) == 1) {
    unsigned __int128 v = 0;
    for (int i = 0; i < 16; ++i) {
      v <<= 8;
      v |= static_cast<unsigned char>(a6.s6_addr[i]);
    }
    return std::make_unique<IPv6Address>(v);
  }
  return nullptr;
}

inline std::unique_ptr<IPAddress> IPAddress::maskFromCIDR(AddressFamily fam,
                                                          uint8_t cidr) {
  if (fam == AddressFamily::IPv4) {
    if (cidr > 32)
      return nullptr;
    uint32_t maskVal = 0;
    if (cidr == 0)
      maskVal = 0;
    else
      maskVal = (cidr == 32) ? ~uint32_t(0) : (~uint32_t(0) << (32 - cidr));
    return std::make_unique<IPv4Address>(maskVal);
  } else if (fam == AddressFamily::IPv6) {
    if (cidr > 128)
      return nullptr;
    unsigned __int128 maskVal = 0;
    if (cidr == 0) {
      maskVal = 0;
    } else if (cidr == 128) {
      maskVal = ~static_cast<unsigned __int128>(0);
    } else {
      unsigned __int128 allOnes = ~static_cast<unsigned __int128>(0);
      maskVal = (allOnes << (128 - cidr));
    }
    return std::make_unique<IPv6Address>(maskVal);
  }
  return nullptr;
}
