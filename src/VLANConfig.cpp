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

#include "VLANConfig.hpp"
#include "ConfigurationManager.hpp"

#include <cerrno>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>

VLANConfig::VLANConfig(const InterfaceConfig &base) {
  name = base.name;
  type = base.type;
  if (base.address)
    address = base.address->clone();
  aliases.clear();
  for (const auto &a : base.aliases) {
    if (a)
      aliases.push_back(a->clone());
  }
  if (base.vrf)
    vrf = std::make_unique<VRFConfig>(*base.vrf);
  flags = base.flags;
  groups = base.groups;
  mtu = base.mtu;
}

VLANConfig::VLANConfig(const InterfaceConfig &base, uint16_t id_,
                       std::optional<std::string> parent_,
                       std::optional<PriorityCodePoint> pcp_) {
  name = base.name;
  type = base.type;
  if (base.address)
    address = base.address->clone();
  aliases.clear();
  for (const auto &a : base.aliases) {
    if (a)
      aliases.push_back(a->clone());
  }
  if (base.vrf)
    vrf = std::make_unique<VRFConfig>(*base.vrf);
  flags = base.flags;
  groups = base.groups;
  mtu = base.mtu;

  id = id_;
  parent = parent_;
  pcp = pcp_;
}

void VLANConfig::save(ConfigurationManager &mgr) const { mgr.SaveVlan(*this); }

void VLANConfig::create(ConfigurationManager &mgr) const {
  mgr.CreateInterface(name);
}

void VLANConfig::destroy(ConfigurationManager &mgr) const {
  mgr.DestroyInterface(name);
}
