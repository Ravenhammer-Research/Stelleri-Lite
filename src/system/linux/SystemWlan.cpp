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
#include "WlanInterfaceConfig.hpp"
#include <string>
#include <vector>

std::vector<WlanInterfaceConfig> SystemConfigurationManager::GetWlanInterfaces(
    const std::vector<InterfaceConfig> &bases) const {
  std::vector<WlanInterfaceConfig> results;
  for (const auto &base : bases) {
    if (base.type == InterfaceType::Wireless) {
      results.emplace_back(base);
      // Detailed attribute retrieval via nl80211 would go here
    }
  }
  return results;
}

void SystemConfigurationManager::CreateWlan(const std::string &name
                                            [[maybe_unused]]) const {
  // WLAN creation on Linux is usually not 'creating' but configuring an
  // existing hardware interface. Unless it's creating a virtual interface (e.g.
  // monitor mode, multiple SSIDs).
}

void SystemConfigurationManager::SaveWlan(const WlanInterfaceConfig &wlan
                                          [[maybe_unused]]) const {
  // Apply SSID, passphrase, mode etc via nl80211 or specialized tools like
  // wpa_supplicant
}
