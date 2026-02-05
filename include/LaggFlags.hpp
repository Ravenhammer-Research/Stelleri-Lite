// LAGG port flag enumerations. Label rendering lives in formatters.
#pragma once

#include <cstdint>

namespace LaggFlags {

enum class PortFlag : uint32_t {
  MASTER = 0x1,
  STACK = 0x2,
  ACTIVE = 0x4,
  COLLECTING = 0x8,
  DISTRIBUTING = 0x10,
};

} // namespace LaggFlags
