#if !defined(STELLERI_NETCONF) || STELLERI_NETCONF != 1
#error "netconf headers are for the STELLERI_NETCONF build only"
#endif

#include "Client.hpp"
#include "IetfInterfaces.hpp"
#include "NetconfConfigurationManager.hpp"
#include "NetconfError.hpp"
#include "YangContext.hpp"
#include "YangData.hpp"
#include "YangError.hpp"
#include <iostream>
#include <libyang/libyang.h>

std::vector<InterfaceConfig> NetconfConfigurationManager::GetInterfaces(
    const std::optional<VRFConfig> & /*vrf*/) const {
  // Use the global Client singleton to request the interfaces subtree.
  Client *c = Client::instance();
  if (!c)
    throw NetconfError(nullptr, YangContext());

  YangContext yctx = c->session().yangContext();
  if (!yctx.get())
    throw YangError(yctx);

  YangModule ym = yctx.GetModule("ietf-interfaces");
  const struct lys_module *mod = ym.getModulePtr();

  if (!mod || !ym.valid()) {
    // Attempt to load it if it's missing from the context.
    mod = ly_ctx_load_module(const_cast<struct ly_ctx *>(yctx.get()),
                             "ietf-interfaces", nullptr, nullptr);
    if (!mod) {
      throw std::runtime_error("ietf-interfaces module not found in client "
                               "context and failed to load");
    }
    // Implement it so we can create data nodes.
    if (lys_set_implemented(const_cast<struct lys_module *>(mod), nullptr) !=
        LY_SUCCESS) {
      throw YangError(yctx);
    }
  }

  struct lyd_node *filter_node = nullptr;
  if (lyd_new_inner(nullptr, mod, "interfaces", 0, &filter_node) !=
      LY_SUCCESS) {
    throw YangError(yctx);
  }

  // Wrap the created filter node in a YangData so its destructor
  // will free the libyang data tree when it goes out of scope.
  YangData filter(filter_node, SubTree);

  // Use the Client helper to perform the get-config with the typed
  // subtree filter (avoid converting to XML and manually sending RPCs).
  std::unique_ptr<NetconfServerReply> reply = c->getConfig(filter);
  std::vector<InterfaceConfig> out;
  if (reply && reply->hasData()) {
    auto data = reply->getData();
    struct lyd_node *op = data ? data->toLydNode() : nullptr;
    if (op) {
      struct ly_set *set = nullptr;
      if (lyd_find_xpath(op, "/ietf-interfaces:interfaces/interface", &set) ==
              LY_SUCCESS &&
          set) {
        for (uint32_t i = 0; i < set->count; ++i) {
          struct lyd_node *n = set->dnodes[i];
          if (!n)
            continue;
          IetfInterfaces ii(n);
          out.push_back(ii.toInterfaceConfig());
        }
        ly_set_free(set, nullptr);
      }
    }
  }

  return out;
}

std::vector<InterfaceConfig> NetconfConfigurationManager::GetInterfacesByGroup(
    const std::optional<VRFConfig> & /*vrf*/,
    std::string_view /*group*/) const {
  return {};
}

void NetconfConfigurationManager::CreateInterface(
    const std::string & /*name*/) const {}
void NetconfConfigurationManager::SaveInterface(
    const InterfaceConfig & /*ic*/) const {}
void NetconfConfigurationManager::DestroyInterface(
    const std::string & /*name*/) const {}
void NetconfConfigurationManager::RemoveInterfaceAddress(
    const std::string & /*ifname*/, const std::string & /*addr*/) const {}
void NetconfConfigurationManager::RemoveInterfaceGroup(
    const std::string & /*ifname*/, const std::string & /*group*/) const {}
bool NetconfConfigurationManager::InterfaceExists(
    std::string_view /*name*/) const {
  return false;
}
std::vector<std::string> NetconfConfigurationManager::GetInterfaceAddresses(
    const std::string & /*ifname*/, int /*family*/) const {
  return {};
}

std::vector<VRFConfig> NetconfConfigurationManager::GetVrfs() const {
  return {};
}

void NetconfConfigurationManager::CreateVrf(const VRFConfig & /*vrf*/) const {}
void NetconfConfigurationManager::DeleteVrf(
    const std::string & /*name*/) const {}
