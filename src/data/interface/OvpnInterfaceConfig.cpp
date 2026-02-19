#include "OvpnInterfaceConfig.hpp"
#include "ConfigurationManager.hpp"
#include "InterfaceType.hpp"

void OvpnInterfaceConfig::save(ConfigurationManager &mgr) const {
  mgr.SaveOvpn(*this);
}

void OvpnInterfaceConfig::create(ConfigurationManager &mgr) const {
  mgr.CreateOvpn(name);
}

OvpnInterfaceConfig::OvpnInterfaceConfig(const InterfaceConfig &base)
    : InterfaceConfig(base) {
  type = InterfaceType::Tunnel;
}

OvpnInterfaceConfig::OvpnInterfaceConfig(const InterfaceConfig &base,
                                         std::unique_ptr<IPAddress> source,
                                         std::unique_ptr<IPAddress> destination)
    : OvpnInterfaceConfig(base) {
  this->source = std::move(source);
  this->destination = std::move(destination);
}
