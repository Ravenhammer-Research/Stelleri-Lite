#include "TunInterfaceConfig.hpp"
#include "ConfigurationManager.hpp"
#include "InterfaceType.hpp"
 
void TunInterfaceConfig::save(ConfigurationManager &mgr) const { mgr.SaveTun(*this); }

void TunInterfaceConfig::create(ConfigurationManager &mgr) const { mgr.CreateTun(name); }

TunInterfaceConfig::TunInterfaceConfig(const InterfaceConfig &base) {
  name = base.name;
  type = InterfaceType::Tun;
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

TunInterfaceConfig::TunInterfaceConfig(const InterfaceConfig &base,
                     std::unique_ptr<IPAddress> source,
                     std::unique_ptr<IPAddress> destination)
    : TunInterfaceConfig(base) {
  this->source = std::move(source);
  this->destination = std::move(destination);
}
