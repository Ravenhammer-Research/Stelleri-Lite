#include "GifInterfaceConfig.hpp"
#include "ConfigurationManager.hpp"
#include "InterfaceType.hpp"

void GifInterfaceConfig::save(ConfigurationManager &mgr) const { mgr.SaveGif(*this); }

void GifInterfaceConfig::create(ConfigurationManager &mgr) const { mgr.CreateGif(name); }

GifInterfaceConfig::GifInterfaceConfig(const InterfaceConfig &base) {
  name = base.name;
  type = InterfaceType::Gif;
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

GifInterfaceConfig::GifInterfaceConfig(const InterfaceConfig &base,
                     std::unique_ptr<IPAddress> source,
                     std::unique_ptr<IPAddress> destination)
    : GifInterfaceConfig(base) {
  this->source = std::move(source);
  this->destination = std::move(destination);
}
