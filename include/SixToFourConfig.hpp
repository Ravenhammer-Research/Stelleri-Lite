#pragma once

#include "InterfaceConfig.hpp"

class SixToFourConfig : public InterfaceConfig {
public:
  explicit SixToFourConfig(const InterfaceConfig &base)
      : InterfaceConfig(base) {}
  void save() const override;
  void create() const;
};
