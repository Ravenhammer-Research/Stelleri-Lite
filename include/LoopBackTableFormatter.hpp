#pragma once

#include "TableFormatter.hpp"
#include <vector>

class LoopBackTableFormatter : public TableFormatter {
public:
  LoopBackTableFormatter() = default;
  std::string format(const std::vector<ConfigData> &items) const override;
};
