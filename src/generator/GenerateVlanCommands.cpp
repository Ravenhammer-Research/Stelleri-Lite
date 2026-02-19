/*
 * Copyright (c) 2026, Ravenhammer Research Inc.
 * All rights reserved.
 */

#include "CommandGenerator.hpp"
#include "InterfaceConfig.hpp"
#include "InterfaceToken.hpp"
#include <algorithm>
#include <iostream>
#include <numeric>
#include <unordered_set>

namespace netcli {

  void
  CommandGenerator::generateVLANs(ConfigurationManager &mgr,
                                  std::set<std::string> &processedInterfaces) {
    auto vlans = mgr.GetVLANInterfaces(mgr.GetInterfaces());

    // Build a set of all VLAN names so we can detect parent VLANs that are
    // themselves VLANs (QinQ).  Sort so that any VLAN whose parent is also
    // in the list is emitted after its parent.
    std::unordered_set<std::string> vlanNames;
    for (const auto &v : vlans)
      vlanNames.insert(v.name);

    // Build a sorted index vector (VlanInterfaceConfig is non-copyable/
    // non-move-assignable because InterfaceConfig contains unique_ptr).
    std::vector<std::size_t> order(vlans.size());
    std::iota(order.begin(), order.end(), 0);
    std::stable_sort(
        order.begin(), order.end(), [&](std::size_t ai, std::size_t bi) {
          const auto &a = vlans[ai];
          const auto &b = vlans[bi];
          // If b's parent is a, then a must come first.
          bool aIsParentOfB = b.parent && *b.parent == a.name;
          bool bIsParentOfA = a.parent && *a.parent == b.name;
          if (aIsParentOfB)
            return true;
          if (bIsParentOfA)
            return false;
          // If a's parent is a VLAN and b's is not, b goes first.
          bool aParentIsVlan = a.parent && vlanNames.count(*a.parent);
          bool bParentIsVlan = b.parent && vlanNames.count(*b.parent);
          if (aParentIsVlan != bParentIsVlan)
            return bParentIsVlan;
          return false;
        });

    for (auto idx : order) {
      const auto &ifc = vlans[idx];
      if (processedInterfaces.count(ifc.name))
        continue;

      std::cout << std::string("set ") +
                       InterfaceToken::toString(
                           const_cast<VlanInterfaceConfig *>(&ifc))
                << "\n";
      processedInterfaces.insert(ifc.name);

      for (const auto &alias : ifc.aliases) {
        InterfaceConfig tmp = ifc;
        tmp.address = alias->clone();
        std::cout << std::string("set ") + InterfaceToken::toString(&tmp)
                  << "\n";
      }
    }
  }

} // namespace netcli
