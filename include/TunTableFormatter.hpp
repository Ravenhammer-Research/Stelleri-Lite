/**
 * TunTableFormatter
 */

#pragma once

#include "TableFormatter.hpp"
#include "TunInterfaceConfig.hpp"
#include <string>
#include <vector>

class TunTableFormatter : public TableFormatter<TunInterfaceConfig> {
public:
  TunTableFormatter() = default;
  std::string
  format(const std::vector<TunInterfaceConfig> &interfaces) override;
};
