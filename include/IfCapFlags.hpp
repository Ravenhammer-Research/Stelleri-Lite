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
 * @file IfCapFlags.hpp
 * @brief Portable enum mirrors for FreeBSD interface capability flags (IFCAP_*)
 */

#pragma once

#include <cstdint>
#include <string>

/// Interface capability flags (mirrors IFCAP_* from <net/if.h>)
enum class IfCap : uint32_t {
  RxCsum = 1u << 0,        // IFCAP_RXCSUM
  TxCsum = 1u << 1,        // IFCAP_TXCSUM
  NetCons = 1u << 2,       // IFCAP_NETCONS
  VlanMtu = 1u << 3,       // IFCAP_VLAN_MTU
  VlanHwTag = 1u << 4,     // IFCAP_VLAN_HWTAGGING
  JumboMtu = 1u << 5,      // IFCAP_JUMBO_MTU
  Polling = 1u << 6,       // IFCAP_POLLING
  VlanHwCsum = 1u << 7,    // IFCAP_VLAN_HWCSUM
  Tso4 = 1u << 8,          // IFCAP_TSO4
  Tso6 = 1u << 9,          // IFCAP_TSO6
  Lro = 1u << 10,          // IFCAP_LRO
  WolUcast = 1u << 11,     // IFCAP_WOL_UCAST
  WolMcast = 1u << 12,     // IFCAP_WOL_MCAST
  WolMagic = 1u << 13,     // IFCAP_WOL_MAGIC
  Toe4 = 1u << 14,         // IFCAP_TOE4
  Toe6 = 1u << 15,         // IFCAP_TOE6
  VlanHwFilter = 1u << 16, // IFCAP_VLAN_HWFILTER
  Nv = 1u << 17,           // IFCAP_NV
  VlanHwTso = 1u << 18,    // IFCAP_VLAN_HWTSO
  LinkState = 1u << 19,    // IFCAP_LINKSTATE
  Netmap = 1u << 20,       // IFCAP_NETMAP
  RxCsumV6 = 1u << 21,     // IFCAP_RXCSUM_IPV6
  TxCsumV6 = 1u << 22,     // IFCAP_TXCSUM_IPV6
  HwStats = 1u << 23,      // IFCAP_HWSTATS
  TxRtLmt = 1u << 24,      // IFCAP_TXRTLMT
  HwRxTstmp = 1u << 25,    // IFCAP_HWRXTSTMP
  MextPg = 1u << 26,       // IFCAP_MEXTPG
  TxTls4 = 1u << 27,       // IFCAP_TXTLS4
  TxTls6 = 1u << 28,       // IFCAP_TXTLS6
  VxlanHwCsum = 1u << 29,  // IFCAP_VXLAN_HWCSUM
  VxlanHwTso = 1u << 30,   // IFCAP_VXLAN_HWTSO
  TxTlsRtLmt = 1u << 31,   // IFCAP_TXTLS_RTLMT
};

inline bool hasIfCap(uint32_t caps, IfCap bit) {
  return (caps & static_cast<uint32_t>(bit)) != 0;
}

/// Format capability bits as a human-readable string like "RXCSUM,LINKSTATE"
inline std::string ifCapToString(uint32_t caps) {
  struct Entry {
    IfCap bit;
    const char *name;
  };
  static constexpr Entry table[] = {
      {IfCap::RxCsum, "RXCSUM"},
      {IfCap::TxCsum, "TXCSUM"},
      {IfCap::NetCons, "NETCONS"},
      {IfCap::VlanMtu, "VLAN_MTU"},
      {IfCap::VlanHwTag, "VLAN_HWTAGGING"},
      {IfCap::JumboMtu, "JUMBO_MTU"},
      {IfCap::Polling, "POLLING"},
      {IfCap::VlanHwCsum, "VLAN_HWCSUM"},
      {IfCap::Tso4, "TSO4"},
      {IfCap::Tso6, "TSO6"},
      {IfCap::Lro, "LRO"},
      {IfCap::WolUcast, "WOL_UCAST"},
      {IfCap::WolMcast, "WOL_MCAST"},
      {IfCap::WolMagic, "WOL_MAGIC"},
      {IfCap::Toe4, "TOE4"},
      {IfCap::Toe6, "TOE6"},
      {IfCap::VlanHwFilter, "VLAN_HWFILTER"},
      {IfCap::Nv, "NV"},
      {IfCap::VlanHwTso, "VLAN_HWTSO"},
      {IfCap::LinkState, "LINKSTATE"},
      {IfCap::Netmap, "NETMAP"},
      {IfCap::RxCsumV6, "RXCSUM_IPV6"},
      {IfCap::TxCsumV6, "TXCSUM_IPV6"},
      {IfCap::HwStats, "HWSTATS"},
      {IfCap::TxRtLmt, "TXRTLMT"},
      {IfCap::HwRxTstmp, "HWRXTSTMP"},
      {IfCap::MextPg, "MEXTPG"},
      {IfCap::TxTls4, "TXTLS4"},
      {IfCap::TxTls6, "TXTLS6"},
      {IfCap::VxlanHwCsum, "VXLAN_HWCSUM"},
      {IfCap::VxlanHwTso, "VXLAN_HWTSO"},
      {IfCap::TxTlsRtLmt, "TXTLS_RTLMT"},
  };
  std::string out;
  for (const auto &e : table) {
    if (hasIfCap(caps, e.bit)) {
      if (!out.empty())
        out += ',';
      out += e.name;
    }
  }
  return out;
}
