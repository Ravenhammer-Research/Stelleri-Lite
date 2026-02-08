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
#include "ConfigurationManager.hpp"
#include <cerrno>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>

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

void BridgeInterfaceConfig::save(ConfigurationManager &mgr) const {
  // Create bridge if it doesn't exist, then apply all settings
  if (!InterfaceConfig::exists(mgr, name)) {
    mgr.CreateBridge(name);
  }

  // Always apply generic interface settings (VRF, MTU, flags, groups, etc.)
  InterfaceConfig::save(mgr);

  // Delegate platform-specific bridge configuration to System layer
  mgr.SaveBridge(*this);
}

void BridgeInterfaceConfig::create(ConfigurationManager &mgr) const {
  if (InterfaceConfig::exists(mgr, name))
    return;
  mgr.CreateBridge(name);
}

void BridgeInterfaceConfig::loadMembers(const ConfigurationManager &mgr) {
  members = mgr.GetBridgeMembers(name);
}
