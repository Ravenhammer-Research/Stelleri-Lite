/*
 * IPsec tunnel/interface configuration
 */

#pragma once

#include "IPAddress.hpp"
#include "InterfaceConfig.hpp"
#include <memory>
#include <optional>
#include <string>
#include "ConfigurationManager.hpp"
#include "InterfaceType.hpp"

class IpsecInterfaceConfig : public InterfaceConfig {
public:
  IpsecInterfaceConfig() = default;
  IpsecInterfaceConfig(const InterfaceConfig &base);
  IpsecInterfaceConfig(const InterfaceConfig &base,
              std::unique_ptr<IPAddress> source,
              std::unique_ptr<IPAddress> destination);

  std::unique_ptr<IPAddress> source;
  std::unique_ptr<IPAddress> destination;
  std::optional<uint32_t> options;
  std::optional<int> tunnel_vrf;

  IpsecInterfaceConfig(const IpsecInterfaceConfig &o) : InterfaceConfig(o), options(o.options),
                                     tunnel_vrf(o.tunnel_vrf) {
    if (o.source)
      source = o.source->clone();
    if (o.destination)
      destination = o.destination->clone();
  }

  void save(ConfigurationManager &mgr) const override;
private:
  void create(ConfigurationManager &mgr) const;
};
// implementations in src/cfg/IpsecInterfaceConfig.cpp
