/**
 * @file LaggConfig.hpp
 * @brief Link aggregation (LAGG) configuration
 */

#pragma once

#include "InterfaceConfig.hpp"
#include <optional>
#include <string>
#include <vector>

#include "LaggProtocol.hpp"

/**
 * @brief Configuration for link aggregation interfaces
 */
class LaggConfig : public InterfaceConfig {
public:
  LaggConfig() = default;
  LaggConfig(const InterfaceConfig &base);
  LaggConfig(const InterfaceConfig &base, LaggProtocol protocol,
             std::vector<std::string> members,
             std::optional<std::string> hash_policy,
             std::optional<int> lacp_rate, std::optional<int> min_links);
  LaggProtocol protocol =
      LaggProtocol::NONE;           ///< LAGG protocol (LACP, failover, etc.)
  std::vector<std::string> members; ///< Member port names
  std::vector<std::string> member_flags; ///< Per-member flag label (e.g., "MASTER")
  std::optional<std::string>
      hash_policy;              ///< Hash policy (layer2, layer2+3, layer3+4)
  std::optional<int> lacp_rate; ///< LACP rate: 0=slow (30s), 1=fast (1s)
  std::optional<int> min_links; ///< Minimum number of active links

  ~LaggConfig() override = default;
  void save() const override;

private:
  void create() const;
};
