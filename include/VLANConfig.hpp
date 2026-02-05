/**
 * @file VLANConfig.hpp
 * @brief VLAN interface configuration
 */

#pragma once

#include "InterfaceConfig.hpp"
#include "PriorityCodePoint.hpp"
#include "VLANProto.hpp"
#include <cstdint>
#include <optional>
#include <string>

/**
 * @brief Configuration for VLAN interfaces
 *
 * Stores VLAN ID and parent interface relationship.
 */
class VLANConfig : public InterfaceConfig {
public:
  VLANConfig() = default;
  VLANConfig(const InterfaceConfig &base);
  VLANConfig(const InterfaceConfig &base, uint16_t id,
             std::optional<std::string> name, std::optional<std::string> parent,
             std::optional<PriorityCodePoint> pcp);

  uint16_t id = 0;                 ///< VLAN ID (1-4094)
  std::optional<std::string> name; ///< Optional VLAN name
  std::optional<std::string>
      parent; ///< Parent interface (e.g., "em0" for em0.100)
  std::optional<PriorityCodePoint> pcp; ///< Priority Code Point (0-7)
  std::optional<VLANProto> proto;   ///< VLAN protocol as enum
  std::optional<uint32_t> options_bits; ///< Raw interface capability bits (IFCAP_*)

  void save() const override;

private:
  void create() const;
};
