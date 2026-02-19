#include "IpsecInterfaceConfig.hpp"
#include "ConfigurationManager.hpp"
#include "InterfaceType.hpp"

void IpsecInterfaceConfig::save(ConfigurationManager &mgr) const {
  mgr.SaveIpsec(*this);
}

void IpsecInterfaceConfig::create(ConfigurationManager &mgr) const {
  mgr.CreateIpsec(name);
}

IpsecInterfaceConfig::IpsecInterfaceConfig(const InterfaceConfig &base)
    : InterfaceConfig(base) {
  type = InterfaceType::IPsec;
}

IpsecInterfaceConfig::IpsecInterfaceConfig(
    const InterfaceConfig &base, std::unique_ptr<IPAddress> source,
    std::unique_ptr<IPAddress> destination)
    : IpsecInterfaceConfig(base) {
  this->source = std::move(source);
  this->destination = std::move(destination);
}
