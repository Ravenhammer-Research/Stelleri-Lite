/**
 * IpsecTableFormatter
 */

#pragma once

#include "IpsecInterfaceConfig.hpp"
#include "TableFormatter.hpp"
#include <string>
#include <vector>

class IpsecTableFormatter : public TableFormatter<IpsecInterfaceConfig> {
public:
  IpsecTableFormatter() = default;
  std::string
  format(const std::vector<IpsecInterfaceConfig> &interfaces) override;
};
