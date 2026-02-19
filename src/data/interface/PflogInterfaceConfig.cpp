#include "PflogInterfaceConfig.hpp"
#include "ConfigurationManager.hpp"
#include "InterfaceType.hpp"

void PflogInterfaceConfig::save(ConfigurationManager &mgr) const {
  mgr.SaveInterface(*this);
}

void PflogInterfaceConfig::create(ConfigurationManager &mgr) const {
  mgr.CreateInterface(name);
}

void PflogInterfaceConfig::destroy(ConfigurationManager &mgr) const {
  mgr.DestroyInterface(name);
}

PflogInterfaceConfig::PflogInterfaceConfig(const InterfaceConfig &base) {
  name = base.name;
  type = InterfaceType::Unknown; // set appropriate type if defined elsewhere
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
