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

#include "SingleLaggSummaryFormatter.hpp"
#include "LaggHash.hpp"
#include "SingleInterfaceSummaryFormatter.hpp"
#include <sstream>

static std::string protocolToString(LaggProtocol proto) {
  switch (proto) {
  case LaggProtocol::LACP:
    return "LACP";
  case LaggProtocol::FAILOVER:
    return "Failover";
  case LaggProtocol::LOADBALANCE:
    return "Load Balance";
  case LaggProtocol::ROUNDROBIN:
    return "Round Robin";
  case LaggProtocol::BROADCAST:
    return "Broadcast";
  case LaggProtocol::NONE:
    return "None";
  default:
    return "Unknown";
  }
}

std::string
SingleLaggSummaryFormatter::format(const LaggInterfaceConfig &lag) const {
  SingleInterfaceSummaryFormatter base;
  std::string out = base.format(lag);

  std::ostringstream oss;
  oss << "Protocol:  " << protocolToString(lag.protocol) << "\n";

  if (lag.hash_policy) {
    std::string hp;
    if (*lag.hash_policy & LaggHash::L2)
      hp += "L2 ";
    if (*lag.hash_policy & LaggHash::L3)
      hp += "L3 ";
    if (*lag.hash_policy & LaggHash::L4)
      hp += "L4 ";
    if (!hp.empty())
      hp.pop_back();
    oss << "Hash:      " << hp << "\n";
  }
  if (lag.lacp_rate)
    oss << "LACP Rate: " << (*lag.lacp_rate == 1 ? "fast" : "slow") << "\n";
  if (lag.min_links)
    oss << "MinLinks:  " << *lag.min_links << "\n";
  if (lag.flowid_shift)
    oss << "FlowShift: " << *lag.flowid_shift << "\n";
  if (lag.rr_stride)
    oss << "RR Stride: " << *lag.rr_stride << "\n";
  if (lag.active_ports)
    oss << "Active:    " << *lag.active_ports << "\n";
  if (lag.flapping)
    oss << "Flapping:  " << *lag.flapping << "\n";

  if (!lag.members.empty()) {
    oss << "Members:   ";
    for (size_t i = 0; i < lag.members.size(); ++i) {
      if (i)
        oss << ", ";
      oss << lag.members[i];
      if (i < lag.member_flags.size() && !lag.member_flags[i].empty())
        oss << " (" << lag.member_flags[i] << ")";
    }
    oss << "\n";
  }

  out += oss.str();
  return out;
}
