#include "IpsecInterfaceConfig.hpp"
#include "ConfigurationManager.hpp"
#include "InterfaceType.hpp"

void IpsecInterfaceConfig::save(ConfigurationManager &mgr) const { mgr.SaveIpsec(*this); }

void IpsecInterfaceConfig::create(ConfigurationManager &mgr) const { mgr.CreateIpsec(name); }

IpsecInterfaceConfig::IpsecInterfaceConfig(const InterfaceConfig &base) {
  name = base.name;
  type = InterfaceType::IPsec;
  if (base.address)
    address = base.address->clone();
  aliases.clear();
  for (const auto &a : base.aliases) {
    if (a)
      aliases.emplace_back(a->clone());
    else
      aliases.emplace_back(nullptr);
  }
  if (base.vrf)
    vrf = std::make_unique<VRFConfig>(*base.vrf);
  flags = base.flags;
  groups = base.groups;
  mtu = base.mtu;
}

IpsecInterfaceConfig::IpsecInterfaceConfig(const InterfaceConfig &base,
                         std::unique_ptr<IPAddress> source,
                         std::unique_ptr<IPAddress> destination)
    : IpsecInterfaceConfig(base) {
  this->source = std::move(source);
  this->destination = std::move(destination);
}
