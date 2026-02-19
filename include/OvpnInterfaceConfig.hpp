/*
 * OVPN tunnel interface configuration
 */

#pragma once

#include "ConfigurationManager.hpp"
#include "IPAddress.hpp"
#include "InterfaceConfig.hpp"
#include "InterfaceType.hpp"
#include <memory>
#include <optional>
#include <string>

class OvpnInterfaceConfig : public InterfaceConfig {
public:
  OvpnInterfaceConfig() = default;
  OvpnInterfaceConfig(const InterfaceConfig &base);
  OvpnInterfaceConfig(const InterfaceConfig &base,
                      std::unique_ptr<IPAddress> source,
                      std::unique_ptr<IPAddress> destination);

  std::unique_ptr<IPAddress> source;
  std::unique_ptr<IPAddress> destination;
  std::optional<uint32_t> options;
  std::optional<int> tunnel_vrf;

  OvpnInterfaceConfig(const OvpnInterfaceConfig &o)
      : InterfaceConfig(o), options(o.options), tunnel_vrf(o.tunnel_vrf) {
    if (o.source)
      source = o.source->clone();
    if (o.destination)
      destination = o.destination->clone();
  }

  void save(ConfigurationManager &mgr) const override;

private:
  void create(ConfigurationManager &mgr) const;
};
