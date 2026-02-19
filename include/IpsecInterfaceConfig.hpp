/*
 * IPsec tunnel/interface configuration
 */

#pragma once

#include "ConfigurationManager.hpp"
#include "IPAddress.hpp"
#include "InterfaceConfig.hpp"
#include "InterfaceType.hpp"
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

/// A single Security Association (SA) entry to install into the kernel SAD.
struct IpsecSA {
  std::string src;       ///< Source address (tunnel outer)
  std::string dst;       ///< Destination address (tunnel outer)
  std::string protocol;  ///< "ah" or "esp"
  uint32_t spi = 0;      ///< Security Parameter Index
  std::string algorithm; ///< Auth algorithm name (e.g. "hmac-md5")
  std::string auth_key;  ///< Authentication key (hex, with or without 0x)
  std::optional<std::string> enc_algorithm; ///< Encryption algorithm (ESP)
  std::optional<std::string> enc_key;       ///< Encryption key (ESP, hex)
};

/// A single Security Policy (SP) entry.
struct IpsecSP {
  std::string direction; ///< "in" or "out"
  std::string policy;    ///< Policy string for ipsec_set_policy(), e.g. "ipsec
                         ///< esp/tunnel/1.2.3.4-5.6.7.8/require"
  std::optional<uint32_t> reqid; ///< SPD policy identifier (-u id)
};

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

  /// Security Associations configured on this interface
  std::vector<IpsecSA> security_associations;

  /// Security Policies configured on this interface
  std::vector<IpsecSP> security_policies;

  /// Interface reqid linking to SPD entries
  std::optional<uint32_t> reqid;

  IpsecInterfaceConfig(const IpsecInterfaceConfig &o)
      : InterfaceConfig(o), options(o.options), tunnel_vrf(o.tunnel_vrf),
        security_associations(o.security_associations),
        security_policies(o.security_policies), reqid(o.reqid) {
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
