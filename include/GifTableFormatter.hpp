/**
 * GifTableFormatter
 */

#pragma once

#include "GifInterfaceConfig.hpp"
#include "TableFormatter.hpp"
#include <string>
#include <vector>

class GifTableFormatter : public TableFormatter<GifInterfaceConfig> {
public:
  GifTableFormatter() = default;
  std::string
  format(const std::vector<GifInterfaceConfig> &interfaces) override;
};
