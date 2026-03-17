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

#include "ArpConfig.hpp"
#include "SystemConfigurationManager.hpp"
#include <arpa/inet.h>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <net/if.h>
#include <net/if_arp.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

std::vector<ArpConfig> SystemConfigurationManager::GetArpEntries(
    const std::optional<std::string> &ip_filter,
    const std::optional<std::string> &iface_filter) const {
  std::vector<ArpConfig> entries;

  std::ifstream arp_file("/proc/net/arp");
  if (!arp_file.is_open())
    return entries;

  std::string line;
  std::getline(arp_file, line); // Skip header

  while (std::getline(arp_file, line)) {
    std::stringstream ss(line);
    std::string ip, hw_type, flags_str, hw_addr, mask, device;
    ss >> ip >> hw_type >> flags_str >> hw_addr >> mask >> device;

    if (ip_filter && *ip_filter != ip)
      continue;
    if (iface_filter && *iface_filter != device)
      continue;

    ArpConfig entry;
    entry.ip = ip;
    entry.mac = hw_addr;
    entry.iface = device;

    uint32_t flags = std::stoul(flags_str, nullptr, 16);
    if (flags & ATF_PERM)
      entry.permanent = true;
    if (flags & ATF_PUBL)
      entry.published = true;

    entries.push_back(std::move(entry));
  }

  return entries;
}

bool SystemConfigurationManager::SetArpEntry(
    const std::string &ip, const std::string &mac,
    const std::optional<std::string> &iface, bool temp, bool pub) const {
  int sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock < 0)
    return false;

  struct arpreq req{};
  struct sockaddr_in *sin = (struct sockaddr_in *)&req.arp_pa;
  sin->sin_family = AF_INET;
  if (inet_pton(AF_INET, ip.c_str(), &sin->sin_addr) != 1) {
    close(sock);
    return false;
  }

  req.arp_ha.sa_family = ARPHRD_ETHER;
  unsigned int b[6];
  if (std::sscanf(mac.c_str(), "%x:%x:%x:%x:%x:%x", &b[0], &b[1], &b[2], &b[3],
                  &b[4], &b[5]) == 6) {
    for (int i = 0; i < 6; ++i)
      req.arp_ha.sa_data[i] = static_cast<unsigned char>(b[i]);
  } else {
    close(sock);
    return false;
  }

  req.arp_flags = ATF_COM;
  if (!temp)
    req.arp_flags |= ATF_PERM;
  if (pub)
    req.arp_flags |= ATF_PUBL;

  if (iface) {
    std::strncpy(req.arp_dev, iface->c_str(), sizeof(req.arp_dev) - 1);
  }

  bool success = (ioctl(sock, SIOCSARP, &req) == 0);
  close(sock);
  return success;
}

bool SystemConfigurationManager::DeleteArpEntry(
    const std::string &ip, const std::optional<std::string> &iface) const {
  int sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock < 0)
    return false;

  struct arpreq req{};
  struct sockaddr_in *sin = (struct sockaddr_in *)&req.arp_pa;
  sin->sin_family = AF_INET;
  if (inet_pton(AF_INET, ip.c_str(), &sin->sin_addr) != 1) {
    close(sock);
    return false;
  }

  if (iface) {
    std::strncpy(req.arp_dev, iface->c_str(), sizeof(req.arp_dev) - 1);
  }

  bool success = (ioctl(sock, SIOCDARP, &req) == 0);
  close(sock);
  return success;
}
