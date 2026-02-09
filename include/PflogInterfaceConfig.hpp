#pragma once

#include "InterfaceConfig.hpp"

class PflogInterfaceConfig : public InterfaceConfig {
public:
  PflogInterfaceConfig() = default;
  explicit PflogInterfaceConfig(const InterfaceConfig &base);

  // Persist / create / destroy
  void save(ConfigurationManager &mgr) const override;
  void create(ConfigurationManager &mgr) const;
  void destroy(ConfigurationManager &mgr) const override;
};
