/**
 * @file VLANProto.hpp
 * @brief VLAN protocol enumeration
 */

#pragma once

#include <cstdint>

enum class VLANProto : uint16_t {
  UNKNOWN = 0,
  DOT1Q = 0x8100,   // 802.1Q
  DOT1AD = 0x88a8,  // 802.1ad (Q-in-Q)
  OTHER = 0xffff,
};

// Note: string rendering of VLANProto moved to formatters (VLANTableFormatter)
