/**
 * @file InterfaceConfig.hpp
 * @brief Network interface configuration structure
 */

#pragma once

#include "ConfigData.hpp"
#include "IPNetwork.hpp"
#include "InterfaceType.hpp"
#include <memory>
#include <optional>
#include <string>
#include <vector>

class BridgeInterfaceConfig;
class LaggConfig;
class TunnelConfig;
class VLANConfig;
#include "VRFConfig.hpp"
class VirtualInterfaceConfig;

#include <ifaddrs.h>
#include <string_view>

/**
 * @brief Complete configuration for a network interface
 *
 * Supports all interface types with type-specific configurations.
 * Optional fields allow for sparse configuration updates.
 */
class InterfaceConfig : public ConfigData {
public:
  InterfaceConfig() = default;
  InterfaceConfig(const struct ifaddrs *ifa);
  InterfaceConfig(std::string name, InterfaceType type,
                  std::unique_ptr<IPNetwork> address,
                  std::vector<std::unique_ptr<IPNetwork>> aliases,
                  std::unique_ptr<VRFConfig> vrf, std::optional<uint32_t> flags,
                  std::optional<int> tunnel_vrf,
                  std::vector<std::string> groups, std::optional<int> mtu);
  // Copy semantics: deep copy owned pointers (defined out-of-line)
  InterfaceConfig(const InterfaceConfig &o);
  InterfaceConfig &operator=(const InterfaceConfig &o);
  std::string name; ///< Interface name (e.g., em0, bridge0)
  InterfaceType type = InterfaceType::Unknown; ///< Interface type
  std::unique_ptr<IPNetwork> address; ///< Primary IP address with prefix
  std::vector<std::unique_ptr<IPNetwork>> aliases; ///< Additional IP addresses
  std::unique_ptr<VRFConfig> vrf;                  ///< VRF membership
  std::optional<uint32_t> flags;   ///< System flags (IFF_UP, IFF_RUNNING, etc.)
  std::optional<int> tunnel_vrf;   ///< FIB ID for tunnel routing lookups
  std::vector<std::string> groups; ///< Interface groups
  std::optional<int> mtu;          ///< Maximum Transmission Unit
  std::optional<int> metric;       ///< Interface metric (if available)
  std::optional<int> index;        ///< Interface numeric index (if available)
  std::optional<std::string> nd6_options; ///< ND6 options string (if available)
  // Wireless-specific attributes (populated for IEEE80211/WLAN interfaces)
  std::optional<std::string> ssid;     ///< Connected SSID (if any)
  std::optional<int> channel;          ///< Channel number (if known)
  std::optional<std::string> bssid;    ///< BSSID (MAC) of associated AP
  std::optional<std::string> parent;   ///< Parent device (e.g., rtwn0)
  std::optional<std::string> authmode; ///< Auth mode string (WPA2, etc.)
  std::optional<std::string> media;    ///< Media description (IEEE 802.11 ...)
  std::optional<std::string> status; ///< User-visible status (associated/down)
  // Type-specific payloads (nullable)
  std::shared_ptr<BridgeInterfaceConfig> bridge;
  std::shared_ptr<LaggConfig> lagg;
  std::shared_ptr<TunnelConfig> tunnel;
  std::shared_ptr<VLANConfig> vlan;
  std::shared_ptr<VirtualInterfaceConfig> virtual_iface;

  // Persist this interface configuration to the system.
  void save() const override;

  // Destroy this interface from the system.
  void destroy() const override;

  // Check whether the named interface exists on the system.
  static bool exists(std::string_view name);

protected:
  // Helper to configure addresses (primary + aliases) using the provided
  // socket and `ifr`. Performs necessary SIOCSIFADDR / SIOCSIFNETMASK calls.
  void configureAddresses(int sock, struct ifreq &ifr) const;

  // Helper to configure MTU using the provided socket and `ifr`.
  void configureMTU(int sock, struct ifreq &ifr) const;

  // Helper to configure flags (bring interface up) using the provided socket
  // and `ifr`.
  void configureFlags(int sock, struct ifreq &ifr) const;

  // (Interface existence check moved to `ConfigData::exists`)
};
