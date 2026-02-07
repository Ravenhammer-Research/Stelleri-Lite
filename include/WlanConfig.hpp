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

#pragma once

#include "InterfaceConfig.hpp"
#include "WlanAuthMode.hpp"

class WlanConfig : public InterfaceConfig {
public:
  explicit WlanConfig(const InterfaceConfig &base) : InterfaceConfig(base) {}
  void save() const override;
  void create() const;
  // Wireless-specific attributes
  std::optional<std::string> ssid;
  std::optional<int> channel;
  std::optional<std::string> bssid;
  std::optional<std::string> parent;
  std::optional<WlanAuthMode> authmode;
  std::optional<std::string> media;
  std::optional<std::string> status;
  // Copy constructor from another WlanConfig
  WlanConfig(const WlanConfig &o) : InterfaceConfig(o) {
    ssid = o.ssid;
    channel = o.channel;
    bssid = o.bssid;
    parent = o.parent;
    authmode = o.authmode;
    media = o.media;
    status = o.status;
  }
  WlanConfig &operator=(const WlanConfig &o) = delete;
};
