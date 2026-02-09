#include "OvpnInterfaceConfig.hpp"
#include "ConfigurationManager.hpp"
#include "InterfaceType.hpp"

void OvpnInterfaceConfig::save(ConfigurationManager &mgr) const { mgr.SaveOvpn(*this); }

void OvpnInterfaceConfig::create(ConfigurationManager &mgr) const { mgr.CreateOvpn(name); }

OvpnInterfaceConfig::OvpnInterfaceConfig(const InterfaceConfig &base) {
  name = base.name;
  type = InterfaceType::Tunnel;
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

OvpnInterfaceConfig::OvpnInterfaceConfig(const InterfaceConfig &base,
                       std::unique_ptr<IPAddress> source,
                       std::unique_ptr<IPAddress> destination)
    : OvpnInterfaceConfig(base) {
  this->source = std::move(source);
  this->destination = std::move(destination);
}
