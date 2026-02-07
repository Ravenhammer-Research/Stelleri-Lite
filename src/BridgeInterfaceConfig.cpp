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
 * @file BridgeInterfaceConfig.cpp
 * @brief System application for bridge interface configuration
 */

#include "BridgeInterfaceConfig.hpp"
#include <cerrno>
#include <cstring>
#include <ifaddrs.h>
#include <iostream>
#include <net/if.h>
#include <net/if_bridgevar.h>
#include <stdexcept>
#include <string>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <net/if_bridgevar.h>

BridgeInterfaceConfig::BridgeInterfaceConfig(const InterfaceConfig &base) {
  // copy common InterfaceConfig fields
  name = base.name;
  type = base.type;
  if (base.address)
    address = base.address->cloneNetwork();
  aliases.clear();
  for (const auto &a : base.aliases) {
    if (a)
      aliases.push_back(a->cloneNetwork());
  }
  if (base.vrf)
    vrf = std::make_unique<VRFConfig>(*base.vrf);
  flags = base.flags;
  groups = base.groups;
  mtu = base.mtu;
}

BridgeInterfaceConfig::BridgeInterfaceConfig(
    const InterfaceConfig &base, bool stp_, bool vlanFiltering_,
    std::vector<std::string> members_,
    std::vector<BridgeMemberConfig> member_configs_,
    std::optional<int> priority_, std::optional<int> hello_time_,
    std::optional<int> forward_delay_, std::optional<int> max_age_,
    std::optional<int> aging_time_, std::optional<int> max_addresses_) {
  // copy common InterfaceConfig fields
  name = base.name;
  type = base.type;
  if (base.address)
    address = base.address->cloneNetwork();
  aliases.clear();
  for (const auto &a : base.aliases) {
    if (a)
      aliases.push_back(a->cloneNetwork());
  }
  if (base.vrf)
    vrf = std::make_unique<VRFConfig>(*base.vrf);
  flags = base.flags;
  groups = base.groups;
  mtu = base.mtu;

  // set bridge specific
  stp = stp_;
  vlanFiltering = vlanFiltering_;
  members = std::move(members_);
  member_configs = std::move(member_configs_);
  priority = priority_;
  hello_time = hello_time_;
  forward_delay = forward_delay_;
  max_age = max_age_;
  aging_time = aging_time_;
  max_addresses = max_addresses_;
}

void BridgeInterfaceConfig::save() const {
  const std::string &name = this->name;

  // If the interface does not exist, create it; otherwise apply generic
  // interface settings via the base class.
  if (!InterfaceConfig::exists(name)) {
    create();
  } else {
    InterfaceConfig::save();
  }

  int sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock < 0) {
    throw std::runtime_error("Failed to create socket: " +
                             std::string(strerror(errno)));
  }

  // Add members to bridge (simple list)
  for (const auto &member : members) {
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
      close(sock);
      throw std::runtime_error("Failed to add member '" + member +
                               "' to bridge '" + name +
                               "': " + std::string(strerror(errno)));
    }
  }

  // Add and configure detailed member configs
  for (const auto &member : member_configs) {
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
      close(sock);
      throw std::runtime_error("Failed to add member '" + member.name +
                               "' to bridge '" + name +
                               "': " + std::string(strerror(errno)));
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
  if (priority) {
    struct ifbrparam param;
    std::memset(&param, 0, sizeof(param));
    param.ifbrp_prio = *priority;

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
  if (hello_time) {
    struct ifbrparam param;
    std::memset(&param, 0, sizeof(param));
    param.ifbrp_hellotime = *hello_time;

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
  if (forward_delay) {
    struct ifbrparam param;
    std::memset(&param, 0, sizeof(param));
    param.ifbrp_fwddelay = *forward_delay;

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
  if (max_age) {
    struct ifbrparam param;
    std::memset(&param, 0, sizeof(param));
    param.ifbrp_maxage = *max_age;

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
  if (aging_time) {
    struct ifbrparam param;
    std::memset(&param, 0, sizeof(param));
    param.ifbrp_ctime = *aging_time;

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
  if (max_addresses) {
    struct ifbrparam param;
    std::memset(&param, 0, sizeof(param));
    param.ifbrp_csize = *max_addresses;

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
  if (stp) {
    // Enable STP on member interfaces via BRDGSIFFLGS
    for (const auto &member : members) {
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

  close(sock);
}

void BridgeInterfaceConfig::create() const {
  // Use base helper to check for existence.
  if (InterfaceConfig::exists(name))
    return;

  int sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock < 0) {
    throw std::runtime_error("Failed to create socket: " +
                             std::string(strerror(errno)));
  }

  struct ifreq ifr;
  std::memset(&ifr, 0, sizeof(ifr));
  std::strncpy(ifr.ifr_name, name.c_str(), IFNAMSIZ - 1);

  if (ioctl(sock, SIOCIFCREATE, &ifr) < 0) {
    close(sock);
    throw std::runtime_error("Failed to create interface '" + name +
                             "': " + std::string(strerror(errno)));
  }

  close(sock);
}

void BridgeInterfaceConfig::loadMembers() {
  members.clear();
  int sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock < 0)
    return;
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
    // Kernel expects ifd.ifd_len to be sizeof(ifbifconf) while the actual
    // buffer pointer/length are passed via ifbic.ifbic_buf/ifbic.ifbic_len.
    ifd.ifd_len = static_cast<int>(sizeof(ifbic));
    ifd.ifd_data = &ifbic;

    if (ioctl(sock, SIOCGDRVSPEC, &ifd) < 0) {
      int e = errno;
      // If ioctl fails with EINVAL on the small buffer, try a larger one.
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

    // If kernel filled the buffer fully, it may have more entries; retry with
    // the larger buffer on the next loop iteration would have already run.
    if (!members.empty() || entries == large_entries)
      break;
  }

  close(sock);
}
