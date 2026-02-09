#pragma once

#include "InterfaceConfig.hpp"

class PfsyncInterfaceConfig : public InterfaceConfig {
public:
  PfsyncInterfaceConfig() = default;
  explicit PfsyncInterfaceConfig(const InterfaceConfig &base);

  // Persist / create / destroy
  void save(ConfigurationManager &mgr) const override;
  void create(ConfigurationManager &mgr) const;
  void destroy(ConfigurationManager &mgr) const override;
};
