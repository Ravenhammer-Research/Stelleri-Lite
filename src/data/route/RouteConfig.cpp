/*
 * RouteConfig system operations (destroy)
 */

#include "RouteConfig.hpp"
#include "ConfigurationManager.hpp"

void RouteConfig::save(ConfigurationManager &mgr) const { mgr.AddRoute(*this); }

void RouteConfig::destroy(ConfigurationManager &mgr) const {
  mgr.DeleteRoute(*this);
}
