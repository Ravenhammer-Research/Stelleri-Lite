#pragma once

#include "InterfaceConfig.hpp"

/**
 * @file LoopBackConfig.hpp
 * @brief Configuration holder for loopback-specific settings
 */

class LoopBackConfig : public InterfaceConfig {
public:
  explicit LoopBackConfig(const InterfaceConfig &base) : InterfaceConfig(base) {}

  // No extra fields for now; placeholder for future loopback-specific options
  void save() const override;
  void create() const;
};
