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

#include "SingleInterfaceSummaryFormatter.hpp"
#include "IPNetwork.hpp"
#include "IPv6Flags.hpp"
#include "IfCapFlags.hpp"
#include "InterfaceConfig.hpp"
#include "InterfaceFlags.hpp"
#include "InterfaceType.hpp"
#include <sstream>

namespace {

  void formatIPv6Annotations(std::ostringstream &oss, const IPNetwork *net) {
    if (!net || net->family() != AddressFamily::IPv6)
      return;
    auto *v6 = dynamic_cast<const IPv6Network *>(net);
    if (!v6)
      return;
    if (v6->scopeid)
      oss << " scopeid 0x" << std::hex << *v6->scopeid << std::dec;
    if (v6->addr_flags) {
      uint32_t f = *v6->addr_flags;
      if (hasIn6(f, In6AddrFlag::Autoconf))
        oss << " autoconf";
      if (hasIn6(f, In6AddrFlag::Temporary))
        oss << " temporary";
      if (hasIn6(f, In6AddrFlag::Deprecated))
        oss << " deprecated";
      if (hasIn6(f, In6AddrFlag::Tentative))
        oss << " tentative";
      if (hasIn6(f, In6AddrFlag::Duplicated))
        oss << " duplicated";
      if (hasIn6(f, In6AddrFlag::Detached))
        oss << " detached";
      if (hasIn6(f, In6AddrFlag::NoDad))
        oss << " no_dad";
      if (hasIn6(f, In6AddrFlag::Anycast))
        oss << " anycast";
      if (hasIn6(f, In6AddrFlag::PreferSource))
        oss << " prefer_source";
    }
    if (v6->pltime && *v6->pltime != 0xffffffff)
      oss << " pltime " << *v6->pltime;
    if (v6->vltime && *v6->vltime != 0xffffffff)
      oss << " vltime " << *v6->vltime;
  }

} // namespace

std::string
SingleInterfaceSummaryFormatter::format(const InterfaceConfig &ic) const {
  std::ostringstream oss;

  oss << "Interface: " << ic.name << "\n";
  oss << "Type:      " << interfaceTypeToString(ic.type) << "\n";

  if (ic.description)
    oss << "Descr:     " << *ic.description << "\n";

  if (ic.hwaddr)
    oss << "HWaddr:    " << *ic.hwaddr << "\n";

  if (ic.flags) {
    std::string status = "-";
    if (*ic.flags & static_cast<uint32_t>(InterfaceFlag::RUNNING)) {
      status = "active";
    } else if (*ic.flags & static_cast<uint32_t>(InterfaceFlag::UP)) {
      status = "no-carrier";
    } else {
      status = "down";
    }
    oss << "Status:    " << status << "\n";
  }

  if (ic.mtu) {
    oss << "MTU:       " << *ic.mtu << "\n";
  }

  if (ic.metric && *ic.metric != 0)
    oss << "Metric:    " << *ic.metric << "\n";

  if (ic.baudrate && *ic.baudrate > 0) {
    uint64_t bps = *ic.baudrate;
    if (bps >= 1'000'000'000)
      oss << "Speed:     " << (bps / 1'000'000'000) << " Gbps\n";
    else if (bps >= 1'000'000)
      oss << "Speed:     " << (bps / 1'000'000) << " Mbps\n";
    else if (bps >= 1'000)
      oss << "Speed:     " << (bps / 1'000) << " Kbps\n";
    else
      oss << "Speed:     " << bps << " bps\n";
  }

  if (ic.link_state) {
    const char *ls = "unknown";
    switch (*ic.link_state) {
    case 1:
      ls = "down";
      break;
    case 2:
      ls = "up";
      break;
    }
    oss << "Link:      " << ls << "\n";
  }

  if (ic.capabilities) {
    std::string caps = ifCapToString(*ic.capabilities);
    if (!caps.empty())
      oss << "Options:   " << caps << "\n";
  }

  if (ic.status_str)
    oss << "Driver:    "
        << *ic.status_str; // status_str typically includes a trailing newline

  if (ic.address) {
    oss << "Address:   " << ic.address->toString();
    formatIPv6Annotations(oss, ic.address.get());
    oss << "\n";
  }

  for (const auto &alias : ic.aliases) {
    oss << "           " << alias->toString();
    formatIPv6Annotations(oss, alias.get());
    oss << "\n";
  }

  if (ic.vrf) {
    oss << "VRF:       " << ic.vrf->table << "\n";
  }

  if (ic.nd6_options) {
    oss << "ND6:       ";
    uint32_t nd = *ic.nd6_options;
    if (hasNd6(nd, Nd6Flag::PerformNud))
      oss << "PERFORMNUD ";
    if (hasNd6(nd, Nd6Flag::AcceptRtadv))
      oss << "ACCEPT_RTADV ";
    if (hasNd6(nd, Nd6Flag::AutoLinklocal))
      oss << "AUTO_LINKLOCAL ";
    if (hasNd6(nd, Nd6Flag::IfDisabled))
      oss << "IFDISABLED ";
    if (hasNd6(nd, Nd6Flag::DontSetIfroute))
      oss << "DONT_SET_IFROUTE ";
    if (hasNd6(nd, Nd6Flag::NoRadr))
      oss << "NO_RADR ";
    if (hasNd6(nd, Nd6Flag::NoPreferIface))
      oss << "NO_PREFER_IFACE ";
    if (hasNd6(nd, Nd6Flag::NoDad))
      oss << "NO_DAD ";
    if (hasNd6(nd, Nd6Flag::Ipv6Only))
      oss << "IPV6_ONLY ";
    oss << "\n";
  }

  if (ic.flags) {
    oss << "Flags:     ";
    if (*ic.flags & static_cast<uint32_t>(InterfaceFlag::UP))
      oss << "UP ";
    if (*ic.flags & static_cast<uint32_t>(InterfaceFlag::BROADCAST))
      oss << "BROADCAST ";
    if (*ic.flags & static_cast<uint32_t>(InterfaceFlag::LOOPBACK))
      oss << "LOOPBACK ";
    if (*ic.flags & static_cast<uint32_t>(InterfaceFlag::POINTOPOINT))
      oss << "POINTOPOINT ";
    if (*ic.flags & static_cast<uint32_t>(InterfaceFlag::RUNNING))
      oss << "RUNNING ";
    if (*ic.flags & static_cast<uint32_t>(InterfaceFlag::MULTICAST))
      oss << "MULTICAST";
    oss << "\n";
  }

  return oss.str();
}
