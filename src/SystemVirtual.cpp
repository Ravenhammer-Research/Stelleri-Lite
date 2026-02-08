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

#include "SystemConfigurationManager.hpp"
#include "VirtualInterfaceConfig.hpp"
#include "Socket.hpp"

#include <cerrno>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/types.h>
#include <sys/linker.h>
std::vector<VirtualInterfaceConfig>
SystemConfigurationManager::GetVirtualInterfaces(
    const std::optional<VRFConfig> &vrf) const {
  auto bases = GetInterfaces(vrf);
  std::vector<VirtualInterfaceConfig> out;
  for (const auto &ic : bases) {
    if (ic.type == InterfaceType::Virtual || ic.type == InterfaceType::Tun) {
      out.emplace_back(ic);
    }
  }
  return out;
}

void SystemConfigurationManager::CreateVirtual(const std::string &nm) const {
  // For epair interfaces, check if the pair already exists
  std::string check_name = nm;
  if (nm.rfind("epair", 0) == 0 && 
      !nm.empty() && 
      nm.back() != 'a' && 
      nm.back() != 'b') {
    check_name = nm + "a";
  }
  if (InterfaceConfig::exists(check_name))
    return;

  Socket sock(AF_INET, SOCK_DGRAM);

  struct ifreq ifr;
  std::memset(&ifr, 0, sizeof(ifr));
  std::strncpy(ifr.ifr_name, nm.c_str(), IFNAMSIZ - 1);

  if (ioctl(sock, SIOCIFCREATE2, &ifr) < 0) {
    int err = errno;
    // Special-case epair clones similar to original logic
    if (err == EINVAL && nm.rfind("epair", 0) == 0) {
      struct ifreq tmp_ifr;
      std::memset(&tmp_ifr, 0, sizeof(tmp_ifr));
      std::strncpy(tmp_ifr.ifr_name, "epair", IFNAMSIZ - 1);
      if (ioctl(sock, SIOCIFCREATE2, &tmp_ifr) < 0) {
        int e = errno;
        // Try to load module and retry
        if (kldfind("if_epair.ko") == -1) {
          int kid = kldload("if_epair.ko");
          if (kid == -1) {
            throw std::runtime_error("Failed to create epair interface: " +
                                     std::string(strerror(e)));
          }
        }
        std::memset(&tmp_ifr, 0, sizeof(tmp_ifr));
        std::strncpy(tmp_ifr.ifr_name, "epair", IFNAMSIZ - 1);
        if (ioctl(sock, SIOCIFCREATE2, &tmp_ifr) < 0) {
          throw std::runtime_error("Failed to create epair interface: " +
                                   std::string(strerror(errno)));
        }
      }

      std::string created = tmp_ifr.ifr_name;
      if (created.empty()) {
        throw std::runtime_error("Failed to determine created epair name");
      }

      std::string targetBase = nm;
      if (!targetBase.empty() && (targetBase.back() == 'a' ||
                                  targetBase.back() == 'b'))
        targetBase.pop_back();

      std::string srcPeerA = created;
      std::string srcPeerB = created;
      if (!srcPeerA.empty() && srcPeerA.back() == 'a')
        srcPeerB.back() = 'b';
      else
        srcPeerB = srcPeerA + "b";

      std::string tgtA = targetBase + "a";
      std::string tgtB = targetBase + "b";

      auto rename_iface = [&](const std::string &cur, const std::string &newn) {
        struct ifreq nr;
        std::memset(&nr, 0, sizeof(nr));
        std::strncpy(nr.ifr_name, cur.c_str(), IFNAMSIZ - 1);
        nr.ifr_data = const_cast<char *>(newn.c_str());
        if (ioctl(sock, SIOCSIFNAME, &nr) < 0) {
          return false;
        }
        return true;
      };

      bool okA = rename_iface(srcPeerA, tgtA);
      bool okB = rename_iface(srcPeerB, tgtB);
      if (!okB && okA) {
        rename_iface(tgtA, srcPeerA);
      }

      if (!okA || !okB) {
        throw std::runtime_error("Failed to create/rename epair interfaces");
      }
      return;
    }

    throw std::runtime_error("Failed to create interface '" + nm + "': " +
                             std::string(strerror(err)));
  }
}

void SystemConfigurationManager::SaveVirtual(const VirtualInterfaceConfig &vic) const {
  // Create virtual interface if it doesn't exist, then apply all settings
  // For epair interfaces, check if the 'a' side exists since epairs come in pairs
  std::string actual_name = vic.name;
  std::string check_name = vic.name;
  if (vic.name.rfind("epair", 0) == 0 && 
      !vic.name.empty() && 
      vic.name.back() != 'a' && 
      vic.name.back() != 'b') {
    check_name = vic.name + "a";
    actual_name = vic.name + "a"; // Operate on the 'a' side
  }
  
  if (!InterfaceConfig::exists(check_name))
    CreateVirtual(vic.name);
  
  // Always call SaveInterface to apply VRF, groups, MTU, etc.
  // Use actual_name which includes 'a' suffix for epairs
  VirtualInterfaceConfig actual_vic = vic;
  actual_vic.name = actual_name;
  SaveInterface(static_cast<const InterfaceConfig &>(actual_vic));
  // Promiscuous or other virtual-specific settings could be applied here
}
