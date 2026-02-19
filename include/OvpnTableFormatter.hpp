/**
 * OvpnTableFormatter
 */

#pragma once

#include "OvpnInterfaceConfig.hpp"
#include "TableFormatter.hpp"
#include <string>
#include <vector>

class OvpnTableFormatter : public TableFormatter<OvpnInterfaceConfig> {
public:
  OvpnTableFormatter() = default;
  std::string
  format(const std::vector<OvpnInterfaceConfig> &interfaces) override;
};
