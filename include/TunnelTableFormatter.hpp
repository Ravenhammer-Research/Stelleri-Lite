/**
 * @file TunnelTableFormatter.hpp
 * @brief Formatter for tunnel interface details
 */

#pragma once

#include "ConfigData.hpp"
#include "TableFormatter.hpp"
#include <string>
#include <vector>

/**
 * @brief Formats tunnel interface configuration as ASCII table
 *
 * Shows tunnel-specific details like source, destination, tunnel-vrf (FIB).
 */
class TunnelTableFormatter : public TableFormatter {
public:
  TunnelTableFormatter() = default;

  /**
   * @brief Format tunnel interfaces into a detailed table
   * @param interfaces List of ConfigData with tunnel configurations
   * @return Formatted ASCII table string
   */
  std::string format(const std::vector<ConfigData> &interfaces) const;
};
