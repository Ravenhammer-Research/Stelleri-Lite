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
#include "SystemConfigurationManager.hpp"
#include "Socket.hpp"

#include <cstring>
#include <iostream>
#include <net/if.h>
#include <net/if_bridgevar.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/sockio.h>

bool SystemConfigurationManager::interfaceIsBridge(
    const std::string &ifname) const {
  try {
    Socket sock(AF_INET, SOCK_DGRAM);

    const size_t entries = 8;
    std::vector<struct ifbreq> buf(entries);
    std::memset(buf.data(), 0, buf.size() * sizeof(struct ifbreq));

    struct ifbifconf ifbic;
    std::memset(&ifbic, 0, sizeof(ifbic));
    ifbic.ifbic_len = static_cast<uint32_t>(buf.size() * sizeof(struct ifbreq));
    ifbic.ifbic_buf = reinterpret_cast<caddr_t>(buf.data());

    struct ifdrv ifd;
    std::memset(&ifd, 0, sizeof(ifd));
    std::strncpy(ifd.ifd_name, ifname.c_str(), IFNAMSIZ - 1);
    ifd.ifd_cmd = BRDGGIFS;
    ifd.ifd_len = static_cast<int>(sizeof(ifbic));
    ifd.ifd_data = &ifbic;

    return (ioctl(sock, SIOCGDRVSPEC, &ifd) == 0);
  } catch (...) {
    return false;
  }
}

std::vector<BridgeInterfaceConfig>
SystemConfigurationManager::GetBridgeInterfaces(
    const std::optional<VRFConfig> &vrf) const {
  auto bases = GetInterfaces(vrf);
  std::vector<BridgeInterfaceConfig> out;
  for (const auto &ic : bases) {
    if (ic.type == InterfaceType::Bridge) {
      BridgeInterfaceConfig bic(ic);
      bic.loadMembers();
      out.emplace_back(std::move(bic));
    }
  }
  return out;
}

void SystemConfigurationManager::CreateBridge(const std::string &name) const {
  // Bridge interfaces are cloned from "bridge" base name
  Socket sock(AF_INET, SOCK_DGRAM);

  struct ifreq ifr;
  std::memset(&ifr, 0, sizeof(ifr));
  std::strncpy(ifr.ifr_name, "bridge", IFNAMSIZ - 1);

  if (ioctl(sock, SIOCIFCREATE, &ifr) < 0) {
    throw std::runtime_error("Failed to create bridge '" + name + "': " +
                             std::string(strerror(errno)));
  }
  
  // The kernel returns the created name in ifr.ifr_name (e.g., "bridge0")
  // If the user requested a specific name and it doesn't match, we could rename,
  // but typically we accept the kernel-assigned name
}

void SystemConfigurationManager::SaveBridge(const BridgeInterfaceConfig &bic) const {
  // Apply bridge-specific settings (members, STP, priorities, etc.)
  const std::string &name = bic.name;

  Socket sock(AF_INET, SOCK_DGRAM);

  // Add members to bridge (simple list)
  for (const auto &member : bic.members) {
    struct ifbreq req;
    std::memset(&req, 0, sizeof(req));
    std::strncpy(req.ifbr_ifsname, member.c_str(), IFNAMSIZ - 1);

    struct ifdrv ifd;
    std::memset(&ifd, 0, sizeof(ifd));
    std::strncpy(ifd.ifd_name, name.c_str(), IFNAMSIZ - 1);
    ifd.ifd_cmd = BRDGADD;
    ifd.ifd_len = sizeof(req);
    ifd.ifd_data = &req;

    if (ioctl(sock, SIOCSDRVSPEC, &ifd) < 0) {
      throw std::runtime_error("Failed to add member '" + member +
                               "' to bridge '" + name + "': " +
                               std::string(strerror(errno)));
    }
  }

  // Add and configure detailed member configs
  for (const auto &member : bic.member_configs) {
    // Add member to bridge
    struct ifbreq req;
    std::memset(&req, 0, sizeof(req));
    std::strncpy(req.ifbr_ifsname, member.name.c_str(), IFNAMSIZ - 1);

    struct ifdrv ifd;
    std::memset(&ifd, 0, sizeof(ifd));
    std::strncpy(ifd.ifd_name, name.c_str(), IFNAMSIZ - 1);
    ifd.ifd_cmd = BRDGADD;
    ifd.ifd_len = sizeof(req);
    ifd.ifd_data = &req;

    if (ioctl(sock, SIOCSDRVSPEC, &ifd) < 0) {
      throw std::runtime_error("Failed to add member '" + member.name +
                               "' to bridge '" + name + "': " +
                               std::string(strerror(errno)));
    }

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
      std::memset(&req, 0, sizeof(req));
      std::strncpy(req.ifbr_ifsname, member.name.c_str(), IFNAMSIZ - 1);
      req.ifbr_ifsflags = flags;

      std::memset(&ifd, 0, sizeof(ifd));
      std::strncpy(ifd.ifd_name, name.c_str(), IFNAMSIZ - 1);
      ifd.ifd_cmd = BRDGSIFFLGS;
      ifd.ifd_len = sizeof(req);
      ifd.ifd_data = &req;

      if (ioctl(sock, SIOCSDRVSPEC, &ifd) < 0) {
        std::cerr << "Warning: Failed to set flags on member '" << member.name
                  << "': " << strerror(errno) << "\n";
      }
    }

    // Configure port priority
    if (member.priority) {
      std::memset(&req, 0, sizeof(req));
      std::strncpy(req.ifbr_ifsname, member.name.c_str(), IFNAMSIZ - 1);
      req.ifbr_priority = *member.priority;

      std::memset(&ifd, 0, sizeof(ifd));
      std::strncpy(ifd.ifd_name, name.c_str(), IFNAMSIZ - 1);
      ifd.ifd_cmd = BRDGSIFPRIO;
      ifd.ifd_len = sizeof(req);
      ifd.ifd_data = &req;

      if (ioctl(sock, SIOCSDRVSPEC, &ifd) < 0) {
        std::cerr << "Warning: Failed to set priority on member '"
                  << member.name << "': " << strerror(errno) << "\n";
      }
    }

    // Configure path cost
    if (member.path_cost) {
      std::memset(&req, 0, sizeof(req));
      std::strncpy(req.ifbr_ifsname, member.name.c_str(), IFNAMSIZ - 1);
      req.ifbr_path_cost = *member.path_cost;

      std::memset(&ifd, 0, sizeof(ifd));
      std::strncpy(ifd.ifd_name, name.c_str(), IFNAMSIZ - 1);
      ifd.ifd_cmd = BRDGSIFCOST;
      ifd.ifd_len = sizeof(req);
      ifd.ifd_data = &req;

      if (ioctl(sock, SIOCSDRVSPEC, &ifd) < 0) {
        std::cerr << "Warning: Failed to set path cost on member '"
                  << member.name << "': " << strerror(errno) << "\n";
      }
    }
  }

  // Configure bridge priority
  if (bic.priority) {
    struct ifbrparam param;
    std::memset(&param, 0, sizeof(param));
    param.ifbrp_prio = *bic.priority;

    struct ifdrv ifd;
    std::memset(&ifd, 0, sizeof(ifd));
    std::strncpy(ifd.ifd_name, name.c_str(), IFNAMSIZ - 1);
    ifd.ifd_cmd = BRDGSPRI;
    ifd.ifd_len = sizeof(param);
    ifd.ifd_data = &param;

    if (ioctl(sock, SIOCSDRVSPEC, &ifd) < 0) {
      std::cerr << "Warning: Failed to set bridge priority: " << strerror(errno)
                << "\n";
    }
  }

  // Configure hello time
  if (bic.hello_time) {
    struct ifbrparam param;
    std::memset(&param, 0, sizeof(param));
    param.ifbrp_hellotime = *bic.hello_time;

    struct ifdrv ifd;
    std::memset(&ifd, 0, sizeof(ifd));
    std::strncpy(ifd.ifd_name, name.c_str(), IFNAMSIZ - 1);
    ifd.ifd_cmd = BRDGSHT;
    ifd.ifd_len = sizeof(param);
    ifd.ifd_data = &param;

    if (ioctl(sock, SIOCSDRVSPEC, &ifd) < 0) {
      std::cerr << "Warning: Failed to set hello time: " << strerror(errno)
                << "\n";
    }
  }

  // Configure forward delay
  if (bic.forward_delay) {
    struct ifbrparam param;
    std::memset(&param, 0, sizeof(param));
    param.ifbrp_fwddelay = *bic.forward_delay;

    struct ifdrv ifd;
    std::memset(&ifd, 0, sizeof(ifd));
    std::strncpy(ifd.ifd_name, name.c_str(), IFNAMSIZ - 1);
    ifd.ifd_cmd = BRDGSFD;
    ifd.ifd_len = sizeof(param);
    ifd.ifd_data = &param;

    if (ioctl(sock, SIOCSDRVSPEC, &ifd) < 0) {
      std::cerr << "Warning: Failed to set forward delay: " << strerror(errno)
                << "\n";
    }
  }

  // Configure max age
  if (bic.max_age) {
    struct ifbrparam param;
    std::memset(&param, 0, sizeof(param));
    param.ifbrp_maxage = *bic.max_age;

    struct ifdrv ifd;
    std::memset(&ifd, 0, sizeof(ifd));
    std::strncpy(ifd.ifd_name, name.c_str(), IFNAMSIZ - 1);
    ifd.ifd_cmd = BRDGSMA;
    ifd.ifd_len = sizeof(param);
    ifd.ifd_data = &param;

    if (ioctl(sock, SIOCSDRVSPEC, &ifd) < 0) {
      std::cerr << "Warning: Failed to set max age: " << strerror(errno)
                << "\n";
    }
  }

  // Configure aging time
  if (bic.aging_time) {
    struct ifbrparam param;
    std::memset(&param, 0, sizeof(param));
    param.ifbrp_ctime = *bic.aging_time;

    struct ifdrv ifd;
    std::memset(&ifd, 0, sizeof(ifd));
    std::strncpy(ifd.ifd_name, name.c_str(), IFNAMSIZ - 1);
    ifd.ifd_cmd = BRDGSTO;
    ifd.ifd_len = sizeof(param);
    ifd.ifd_data = &param;

    if (ioctl(sock, SIOCSDRVSPEC, &ifd) < 0) {
      std::cerr << "Warning: Failed to set aging time: " << strerror(errno)
                << "\n";
    }
  }

  // Configure max addresses
  if (bic.max_addresses) {
    struct ifbrparam param;
    std::memset(&param, 0, sizeof(param));
    param.ifbrp_csize = *bic.max_addresses;

    struct ifdrv ifd;
    std::memset(&ifd, 0, sizeof(ifd));
    std::strncpy(ifd.ifd_name, name.c_str(), IFNAMSIZ - 1);
    ifd.ifd_cmd = BRDGSCACHE;
    ifd.ifd_len = sizeof(param);
    ifd.ifd_data = &param;

    if (ioctl(sock, SIOCSDRVSPEC, &ifd) < 0) {
      std::cerr << "Warning: Failed to set max addresses: " << strerror(errno)
                << "\n";
    }
  }

  // Configure STP if requested
  if (bic.stp) {
    for (const auto &member : bic.members) {
      struct ifbreq req;
      std::memset(&req, 0, sizeof(req));
      std::strncpy(req.ifbr_ifsname, member.c_str(), IFNAMSIZ - 1);
      req.ifbr_ifsflags = IFBIF_STP;

      struct ifdrv ifd;
      std::memset(&ifd, 0, sizeof(ifd));
      std::strncpy(ifd.ifd_name, name.c_str(), IFNAMSIZ - 1);
      ifd.ifd_cmd = BRDGSIFFLGS;
      ifd.ifd_len = sizeof(req);
      ifd.ifd_data = &req;

      if (ioctl(sock, SIOCSDRVSPEC, &ifd) < 0) {
        std::cerr << "Warning: Failed to enable STP on member '" << member
                  << "': " << strerror(errno) << "\n";
      }
    }
  }
}

std::vector<std::string> SystemConfigurationManager::GetBridgeMembers(
    const std::string &name) const {
  std::vector<std::string> members;
  try {
    Socket sock(AF_INET, SOCK_DGRAM);

    // Try with a small buffer first, then retry with a larger buffer if needed.
    const size_t small_entries = 64;
    const size_t large_entries = 1024;
    for (size_t entries : {small_entries, large_entries}) {
      std::vector<struct ifbreq> buf(entries);
      std::memset(buf.data(), 0, buf.size() * sizeof(struct ifbreq));

      struct ifbifconf ifbic;
      std::memset(&ifbic, 0, sizeof(ifbic));
      ifbic.ifbic_len = static_cast<uint32_t>(buf.size() * sizeof(struct ifbreq));
      ifbic.ifbic_buf = reinterpret_cast<caddr_t>(buf.data());

      struct ifdrv ifd;
      std::memset(&ifd, 0, sizeof(ifd));
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
  } catch (...) {
    // Socket creation failed, return empty
  }
  return members;
}
