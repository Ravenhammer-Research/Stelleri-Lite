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

#include "BridgeInterfaceConfig.hpp"
#include "Socket.hpp"
#include "SystemConfigurationManager.hpp"

#include <cstring>
#include <iostream>
#include <net/if.h>
#include <net/if_bridgevar.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/sockio.h>

namespace {

  // Issue a bridge driver ioctl with an ifbreq payload.
  bool bridgeMemberIoctl(int sock, const std::string &bridge, unsigned long cmd,
                         struct ifbreq &req, const std::string &context) {
    struct ifdrv ifd{};
    std::strncpy(ifd.ifd_name, bridge.c_str(), IFNAMSIZ - 1);
    ifd.ifd_cmd = cmd;
    ifd.ifd_len = sizeof(req);
    ifd.ifd_data = &req;
    if (ioctl(sock, SIOCSDRVSPEC, &ifd) < 0) {
      std::cerr << "Warning: " << context << ": " << strerror(errno) << "\n";
      return false;
    }
    return true;
  }

  // Issue a bridge driver ioctl with an ifbrparam payload.
  bool bridgeParamIoctl(int sock, const std::string &bridge, unsigned long cmd,
                        struct ifbrparam &param, const std::string &context) {
    struct ifdrv ifd{};
    std::strncpy(ifd.ifd_name, bridge.c_str(), IFNAMSIZ - 1);
    ifd.ifd_cmd = cmd;
    ifd.ifd_len = sizeof(param);
    ifd.ifd_data = &param;
    if (ioctl(sock, SIOCSDRVSPEC, &ifd) < 0) {
      std::cerr << "Warning: " << context << ": " << strerror(errno) << "\n";
      return false;
    }
    return true;
  }

} // namespace

std::vector<BridgeInterfaceConfig>
SystemConfigurationManager::GetBridgeInterfaces(
    const std::vector<InterfaceConfig> &bases) const {
  std::vector<BridgeInterfaceConfig> out;
  for (const auto &ic : bases) {
    if (ic.type == InterfaceType::Bridge) {
      BridgeInterfaceConfig bic(ic);
      bic.loadMembers(*this);
      out.emplace_back(std::move(bic));
    }
  }
  return out;
}

void SystemConfigurationManager::CreateBridge(const std::string &name) const {
  // Bridge interfaces are cloned from "bridge" base name
  Socket sock(AF_INET, SOCK_DGRAM);

  struct ifreq ifr;
  prepare_ifreq(ifr, "bridge");

  if (ioctl(sock, SIOCIFCREATE, &ifr) < 0) {
    throw std::runtime_error("Failed to create bridge '" + name +
                             "': " + std::string(strerror(errno)));
  }

  // The kernel returns the created name in ifr.ifr_name (e.g., "bridge0")
  // If the user requested a specific name and it doesn't match, we could
  // rename, but typically we accept the kernel-assigned name
}

void SystemConfigurationManager::SaveBridge(
    const BridgeInterfaceConfig &bic) const {
  const std::string &name = bic.name;

  Socket sock(AF_INET, SOCK_DGRAM);

  // Helper: add a member to the bridge.
  auto addMember = [&](const std::string &member) {
    struct ifbreq req{};
    std::strncpy(req.ifbr_ifsname, member.c_str(), IFNAMSIZ - 1);
    if (!bridgeMemberIoctl(sock, name, BRDGADD, req,
                           "Failed to add member '" + member + "' to bridge '" +
                               name + "'")) {
      throw std::runtime_error("Failed to add member '" + member +
                               "' to bridge '" + name +
                               "': " + std::string(strerror(errno)));
    }
  };

  // Add members to bridge (simple list)
  for (const auto &member : bic.members)
    addMember(member);

  // Add and configure detailed member configs
  for (const auto &member : bic.member_configs) {
    addMember(member.name);

    // Configure member port flags
    uint32_t flags = 0;
    if (member.stp)
      flags |= IFBIF_STP;
    if (member.edge)
      flags |= IFBIF_BSTP_EDGE;
    if (member.autoedge)
      flags |= IFBIF_BSTP_AUTOEDGE;
    if (member.ptp)
      flags |= IFBIF_BSTP_PTP;
    if (member.autoptp)
      flags |= IFBIF_BSTP_AUTOPTP;

    if (flags > 0) {
      struct ifbreq req{};
      std::strncpy(req.ifbr_ifsname, member.name.c_str(), IFNAMSIZ - 1);
      req.ifbr_ifsflags = flags;
      bridgeMemberIoctl(sock, name, BRDGSIFFLGS, req,
                        "Failed to set flags on member '" + member.name + "'");
    }

    // Configure port priority
    if (member.priority) {
      struct ifbreq req{};
      std::strncpy(req.ifbr_ifsname, member.name.c_str(), IFNAMSIZ - 1);
      req.ifbr_priority = *member.priority;
      bridgeMemberIoctl(sock, name, BRDGSIFPRIO, req,
                        "Failed to set priority on member '" + member.name +
                            "'");
    }

    // Configure path cost
    if (member.path_cost) {
      struct ifbreq req{};
      std::strncpy(req.ifbr_ifsname, member.name.c_str(), IFNAMSIZ - 1);
      req.ifbr_path_cost = *member.path_cost;
      bridgeMemberIoctl(sock, name, BRDGSIFCOST, req,
                        "Failed to set path cost on member '" + member.name +
                            "'");
    }
  }

  // Configure bridge-level parameters via ifbrparam ioctls.
  auto setParam = [&](bool has_val, auto assign_fn, unsigned long cmd,
                      const std::string &label) {
    if (!has_val)
      return;
    struct ifbrparam param{};
    assign_fn(param);
    bridgeParamIoctl(sock, name, cmd, param, "Failed to set " + label);
  };

  setParam(
      !!bic.priority,
      [&](struct ifbrparam &p) { p.ifbrp_prio = *bic.priority; }, BRDGSPRI,
      "bridge priority");
  setParam(
      !!bic.hello_time,
      [&](struct ifbrparam &p) { p.ifbrp_hellotime = *bic.hello_time; },
      BRDGSHT, "hello time");
  setParam(
      !!bic.forward_delay,
      [&](struct ifbrparam &p) { p.ifbrp_fwddelay = *bic.forward_delay; },
      BRDGSFD, "forward delay");
  setParam(
      !!bic.max_age,
      [&](struct ifbrparam &p) { p.ifbrp_maxage = *bic.max_age; }, BRDGSMA,
      "max age");
  setParam(
      !!bic.aging_time,
      [&](struct ifbrparam &p) { p.ifbrp_ctime = *bic.aging_time; }, BRDGSTO,
      "aging time");
  setParam(
      !!bic.max_addresses,
      [&](struct ifbrparam &p) { p.ifbrp_csize = *bic.max_addresses; },
      BRDGSCACHE, "max addresses");

  // Configure STP if requested
  if (bic.stp) {
    for (const auto &member : bic.members) {
      struct ifbreq req{};
      std::strncpy(req.ifbr_ifsname, member.c_str(), IFNAMSIZ - 1);
      req.ifbr_ifsflags = IFBIF_STP;
      bridgeMemberIoctl(sock, name, BRDGSIFFLGS, req,
                        "Failed to enable STP on member '" + member + "'");
    }
  }
}

std::vector<std::string>
SystemConfigurationManager::GetBridgeMembers(const std::string &name) const {
  std::vector<std::string> members;
  Socket sock(AF_INET, SOCK_DGRAM);

  // Try with a small buffer first, then retry with a larger buffer if needed.
  const size_t small_entries = 64;
  const size_t large_entries = 1024;
  for (size_t entries : {small_entries, large_entries}) {
    std::vector<struct ifbreq> buf(entries);

    struct ifbifconf ifbic{};
    ifbic.ifbic_len = static_cast<uint32_t>(buf.size() * sizeof(struct ifbreq));
    ifbic.ifbic_buf = reinterpret_cast<caddr_t>(buf.data());

    struct ifdrv ifd{};
    std::strncpy(ifd.ifd_name, name.c_str(), IFNAMSIZ - 1);
    ifd.ifd_cmd = BRDGGIFS;
    ifd.ifd_len = static_cast<int>(sizeof(ifbic));
    ifd.ifd_data = &ifbic;

    if (ioctl(sock, SIOCGDRVSPEC, &ifd) < 0) {
      int e = errno;
      if (entries == large_entries || e != EINVAL)
        break;
      continue;
    }

    size_t used_bytes = static_cast<size_t>(ifbic.ifbic_len);
    size_t count = (used_bytes / sizeof(struct ifbreq));
    if (count > buf.size())
      count = buf.size();

    for (size_t i = 0; i < count; ++i) {
      if (buf[i].ifbr_ifsname[0] != '\0')
        members.emplace_back(std::string(buf[i].ifbr_ifsname));
    }

    if (!members.empty() || entries == large_entries)
      break;
  }
  return members;
}
