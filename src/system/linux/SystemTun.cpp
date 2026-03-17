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
#include "TunInterfaceConfig.hpp"
#include <cstring>
#include <fcntl.h>
#include <linux/if_tun.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <unistd.h>

void SystemConfigurationManager::CreateTun(const std::string &name) const {
  int fd = open("/dev/net/tun", O_RDWR);
  if (fd < 0)
    return;

  struct ifreq ifr{};
  ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
  if (!name.empty()) {
    std::strncpy(ifr.ifr_name, name.c_str(), IFNAMSIZ - 1);
  }

  ioctl(fd, TUNSETIFF, &ifr);
  // In Linux, the interface usually disappears when the FD is closed,
  // unless TUNSETPERSIST is used.
  ioctl(fd, TUNSETPERSIST, 1);
  close(fd);
}

void SystemConfigurationManager::SaveTun(const TunInterfaceConfig &tun
                                         [[maybe_unused]]) const {
  if (!InterfaceExists(tun.name)) {
    CreateTun(tun.name);
  }
}

std::vector<TunInterfaceConfig> SystemConfigurationManager::GetTunInterfaces(
    const std::vector<InterfaceConfig> &bases [[maybe_unused]]) const {
  std::vector<TunInterfaceConfig> out;
  for (const auto &ic : bases) {
    if (ic.type == InterfaceType::Tun) {
      out.emplace_back(ic);
    }
  }
  return out;
}
