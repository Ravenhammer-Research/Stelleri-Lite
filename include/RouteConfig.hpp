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
 * @file RouteConfig.hpp
 * @brief Route configuration structure
 */

#pragma once

#include "ConfigData.hpp"
#include <optional>
#include <string>

/**
 * @brief Configuration for a routing table entry
 */
class RouteConfig : public ConfigData {
public:
  std::string prefix;                 ///< Destination prefix in CIDR notation
  std::optional<std::string> nexthop; ///< Next-hop IP address
  std::optional<std::string> iface;   ///< Outgoing interface name
  std::optional<int> vrf;             ///< VRF table ID for route
  bool blackhole = false;             ///< Blackhole route (drop packets)
  bool reject = false;                ///< Reject route (send ICMP unreachable)
  std::optional<std::string>
      scope;                 ///< Optional scope/interface for scoped addresses
  std::optional<int> expire; ///< Optional expire time (seconds)
  unsigned int flags = 0;    ///< raw rtm_flags from kernel

  // Route flags and RTAX indices are provided via the enums below which
  // mirror the platform's RTF_* and RTAX_* definitions.

  // Additional route flags (mirror kernel RTF_* values). Values use plain
  // identifiers to match your request (no RTF_ prefix).
  enum RouteFlag : unsigned int {
    UP = 0x1,             /* RTF_UP */
    GATEWAY = 0x2,        /* RTF_GATEWAY */
    HOST = 0x4,           /* RTF_HOST */
    REJECT = 0x8,         /* RTF_REJECT */
    DYNAMIC = 0x10,       /* RTF_DYNAMIC */
    MODIFIED = 0x20,      /* RTF_MODIFIED */
    DONE = 0x40,          /* RTF_DONE */
    XRESOLVE = 0x200,     /* RTF_XRESOLVE */
    LLINFO = 0x400,       /* RTF_LLINFO (deprecated alias) */
    LLDATA = 0x400,       /* RTF_LLDATA */
    STATIC = 0x800,       /* RTF_STATIC */
    BLACKHOLE = 0x1000,   /* RTF_BLACKHOLE */
    PROTO2 = 0x4000,      /* RTF_PROTO2 */
    PROTO1 = 0x8000,      /* RTF_PROTO1 */
    PROTO3 = 0x40000,     /* RTF_PROTO3 */
    FIXEDMTU = 0x80000,   /* RTF_FIXEDMTU */
    PINNED = 0x100000,    /* RTF_PINNED */
    LOCAL = 0x200000,     /* RTF_LOCAL */
    BROADCAST = 0x400000, /* RTF_BROADCAST */
    MULTICAST = 0x800000, /* RTF_MULTICAST */
    STICKY = 0x10000000,  /* RTF_STICKY */
    GWFLAG_COMPAT = 0x80000000 /* RTF_GWFLAG_COMPAT */
  };

  // RTAX index constants (sockaddr array indices). Put into a small nested
  // type to avoid polluting the RouteFlag enumerator names. Use as
  // `RouteConfig::RTAX::DST`, etc.
  enum class RTAX : int {
    DST = 0,
    GATEWAY = 1,
    NETMASK = 2,
    GENMASK = 3,
    IFP = 4,
    IFA = 5,
    AUTHOR = 6,
    BRD = 7,
    COUNT = 8
  };

  // Helper to get int index for RTAX enum values without verbose casts.
  static constexpr int RTAX(RTAX r) { return static_cast<int>(r); }

  // Helper to convert enum RouteFlag to the raw unsigned int flag value.
  static constexpr unsigned int Flag(RouteFlag f) { return static_cast<unsigned int>(f); }

  // Additional collected metadata filled by SystemRoutes.cpp
  std::optional<int> iface_index;           ///< interface index (rtm_index)
  std::optional<std::string> ifa;           ///< interface address (RTAX_IFA)
  std::optional<std::string> ifp;           ///< interface link info/name (RTAX_IFP)
  std::optional<std::string> gateway_hw;    ///< gateway link-layer address (MAC)

  // Route metric copies from rtm_rmx
  unsigned long rmx_mtu = 0;
  unsigned long rmx_hopcount = 0;
  unsigned long rmx_rtt = 0;
  unsigned long rmx_rttvar = 0;
  unsigned long rmx_recvpipe = 0;
  unsigned long rmx_sendpipe = 0;
  unsigned long rmx_ssthresh = 0;
  unsigned long rmx_pksent = 0;

  // Raw routing message provenance
  std::optional<int> rtm_type;
  std::optional<int> rtm_pid;
  std::optional<int> rtm_seq;
  std::optional<int> rtm_msglen;

  // Optional ADDR entries
  std::optional<std::string> author; ///< RTAX::AUTHOR
  std::optional<std::string> brd;    ///< RTAX::BRD

  

  // Persist route configuration via the supplied manager.
  void save(ConfigurationManager &mgr) const override;
  void destroy(ConfigurationManager &mgr) const override;
};
