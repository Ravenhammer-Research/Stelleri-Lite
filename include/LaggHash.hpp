/**
 * @file LaggHash.hpp
 * @brief LAGG hash policy flags
 */

#pragma once

#include <cstdint>
#include <net/if_lagg.h>

namespace LaggHash {

enum Flag : uint32_t {
  L2 = LAGG_F_HASHL2,
  L3 = LAGG_F_HASHL3,
  L4 = LAGG_F_HASHL4,
};

} // namespace LaggHash
