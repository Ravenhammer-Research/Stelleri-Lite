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

/**
 * @file NdpConfig.hpp
 * @brief NDP (IPv6 Neighbor Discovery) table entry configuration
 */

#pragma once

#include "ConfigData.hpp"
#include <optional>
#include <string>
// Avoid pulling system/FreeBSD headers into this public header.
// The icmp6 constants are referenced below as plain integers so that
// `NdpConfig.hpp` remains portable; the platform headers are included
// in the system implementation files (e.g., SystemNdp.cpp).

class NdpConfig : public ConfigData {
public:
  std::string ip;                   // IPv6 address
  std::string mac;                  // MAC address
  std::optional<std::string> iface; // Interface name
  std::optional<int> expire;        // Expiration time
  bool permanent = false;           // Static/permanent entry
  bool router = false;              // Router flag
  unsigned int flags = 0;           // raw flags (rtm_flags or icmp flags)
  std::optional<int> ifindex;       // interface index from sockaddr_dl
  std::optional<int> sdl_alen;      // link-layer address length
  bool has_lladdr = false;          // whether a link-layer addr was present

  // Neighbor Advertisement flags (values per RFC/FreeBSD's icmp6.h).
  // Use numeric literals here so we don't need to include system
  // headers from this public header file.
  enum NeighborFlag : unsigned int {
    NEIGHBOR_ROUTER = 0x80,    // ND_NA_FLAG_ROUTER
    NEIGHBOR_SOLICITED = 0x40, // ND_NA_FLAG_SOLICITED
    NEIGHBOR_OVERRIDE = 0x20   // ND_NA_FLAG_OVERRIDE
  };

  // ND option types (ND_OPT_*) for presence checks
  // ND option type numbers (ND_OPT_*). Kept as literals to avoid
  // requiring system headers in this header file.
  enum OptionType : int {
    OPT_SOURCE_LINKADDR = 1,   // ND_OPT_SOURCE_LINKADDR
    OPT_TARGET_LINKADDR = 2,   // ND_OPT_TARGET_LINKADDR
    OPT_PREFIX_INFORMATION = 3,// ND_OPT_PREFIX_INFORMATION
    OPT_REDIRECTED_HEADER = 4, // ND_OPT_REDIRECTED_HEADER
    OPT_MTU = 5,               // ND_OPT_MTU
    OPT_NONCE = 14,            // ND_OPT_NONCE
    OPT_ROUTE_INFO = 24,       // ND_OPT_ROUTE_INFO
    OPT_RDNSS = 25,            // ND_OPT_RDNSS
    OPT_DNSSL = 31,            // ND_OPT_DNSSL
    OPT_PREF64 = 38            // ND_OPT_PREF64
  };

  // rtm_rmx metric copies
  unsigned long rmx_expire = 0;
  unsigned long rmx_mtu = 0;
  unsigned long rmx_hopcount = 0;
  unsigned long rmx_rtt = 0;
  unsigned long rmx_rttvar = 0;
  unsigned long rmx_recvpipe = 0;
  unsigned long rmx_sendpipe = 0;
  unsigned long rmx_ssthresh = 0;
  unsigned long rmx_pksent = 0;
  int rmx_weight = 0;               // ND6_LLINFO_* state (renamed to avoid macro collision)
  bool is_proxy = false;           // whether this entry is a proxy (RTF_ANNOUNCE)

  // Raw routing message provenance
  std::optional<int> rtm_type;
  std::optional<int> rtm_pid;
  std::optional<int> rtm_seq;
  std::optional<int> rtm_msglen;

  void save(ConfigurationManager &mgr) const override;
  void destroy(ConfigurationManager &mgr) const override;
};
