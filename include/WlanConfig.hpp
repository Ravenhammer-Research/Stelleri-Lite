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
