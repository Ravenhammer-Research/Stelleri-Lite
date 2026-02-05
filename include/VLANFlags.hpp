// VLANFlags.hpp
// Enum and helpers for VLAN/interface capability bits.
#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <net/if.h>

namespace netcli {

enum class VLANFlag : uint32_t {
  RXCSUM = IFCAP_RXCSUM,
  TXCSUM = IFCAP_TXCSUM,
  LINKSTATE = IFCAP_LINKSTATE,
  VLAN_HWTAG = IFCAP_B_VLAN_HWTAGGING,
};

inline constexpr uint32_t to_mask(VLANFlag f) {
  return static_cast<uint32_t>(f);
}

inline bool has_flag(uint32_t mask, VLANFlag f) {
  return (mask & to_mask(f)) != 0;
}


} // namespace netcli
