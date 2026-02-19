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

// FreeBSD LAGG configuration implementation moved out of the manager

// LAGG configuration implementation (moved from manager)

#include "LaggInterfaceConfig.hpp"

#include "ConfigurationManager.hpp"

#include <cerrno>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>

LaggInterfaceConfig::LaggInterfaceConfig(const InterfaceConfig &base)
    : InterfaceConfig(base) {
  type = InterfaceType::Lagg;
}

LaggInterfaceConfig::LaggInterfaceConfig(const InterfaceConfig &base,
                                         LaggProtocol protocol_,
                                         std::vector<std::string> members_,
                                         std::optional<uint32_t> hash_policy_,
                                         std::optional<int> lacp_rate_,
                                         std::optional<int> min_links_)
    : LaggInterfaceConfig(base) {
  protocol = protocol_;
  members = std::move(members_);
  hash_policy = hash_policy_;
  lacp_rate = lacp_rate_;
  min_links = min_links_;
}

void LaggInterfaceConfig::save(ConfigurationManager &mgr) const {
  mgr.SaveLagg(*this);
}

void LaggInterfaceConfig::create(ConfigurationManager &mgr) const {
  mgr.CreateLagg(name);
}
