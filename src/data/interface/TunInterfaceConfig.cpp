#include "TunInterfaceConfig.hpp"
#include "ConfigurationManager.hpp"
#include "InterfaceType.hpp"

void TunInterfaceConfig::save(ConfigurationManager &mgr) const {
  mgr.SaveTun(*this);
}

void TunInterfaceConfig::create(ConfigurationManager &mgr) const {
  mgr.CreateTun(name);
}

TunInterfaceConfig::TunInterfaceConfig(const InterfaceConfig &base)
    : InterfaceConfig(base) {
  type = InterfaceType::Tun;
}

TunInterfaceConfig::TunInterfaceConfig(const InterfaceConfig &base,
                                       std::unique_ptr<IPAddress> source,
                                       std::unique_ptr<IPAddress> destination)
    : TunInterfaceConfig(base) {
  this->source = std::move(source);
  this->destination = std::move(destination);
}
