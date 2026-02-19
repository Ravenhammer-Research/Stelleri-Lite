/*
 * Copyright (c) 2026, Ravenhammer Research Inc.
 * All rights reserved.
 */

/**
 * @file WireGuardInterfaceConfig.hpp
 * @brief WireGuard tunnel interface configuration
 *
 * WireGuard interfaces on FreeBSD (if_wg(4)) expose a minimal kernel
 * surface — most of the cryptographic configuration (peers, keys,
 * allowed-IPs) is managed by userspace tooling (wg(8)/wg-quick(8)).
 *
 * This config class therefore only stores the attributes that are
 * visible through the normal ifconfig / ioctl interface layer: the
 * listening UDP port and the interface-level WireGuard-specific
 * parameters that the kernel exposes.  Peer configuration is out of
 * scope for the interface config generator.
 */

#pragma once

#include "ConfigurationManager.hpp"
#include "InterfaceConfig.hpp"
#include <cstdint>
#include <optional>

class WireGuardInterfaceConfig : public InterfaceConfig {
public:
  WireGuardInterfaceConfig() = default;
  explicit WireGuardInterfaceConfig(const InterfaceConfig &base)
      : InterfaceConfig(base) {}

  /// UDP listen port (0 = random / not set)
  std::optional<uint16_t> listenPort;

  void save(ConfigurationManager &mgr) const override;
  void create(ConfigurationManager &mgr) const;
  void destroy(ConfigurationManager &mgr) const override;
};
