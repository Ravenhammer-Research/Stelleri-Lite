#include "GifInterfaceConfig.hpp"
#include "ConfigurationManager.hpp"
#include "InterfaceType.hpp"

void GifInterfaceConfig::save(ConfigurationManager &mgr) const {
  mgr.SaveGif(*this);
}

void GifInterfaceConfig::create(ConfigurationManager &mgr) const {
  mgr.CreateGif(name);
}

GifInterfaceConfig::GifInterfaceConfig(const InterfaceConfig &base)
    : InterfaceConfig(base) {
  type = InterfaceType::Gif;
}

GifInterfaceConfig::GifInterfaceConfig(const InterfaceConfig &base,
                                       std::unique_ptr<IPAddress> source,
                                       std::unique_ptr<IPAddress> destination)
    : GifInterfaceConfig(base) {
  this->source = std::move(source);
  this->destination = std::move(destination);
}
