/*
 * IetfInterfaces.hpp
 * YANG `ietf-interfaces` data model wrapper (header only)
 */

#pragma once

#include "YangData.hpp"

namespace netcli {

class IetfInterfaces : public YangData {
public:
  explicit IetfInterfaces(struct lyd_node *node = nullptr) : YangData(node) {}
  ~IetfInterfaces() override = default;

  // YangData interface

  // TODO: add parsed fields and helpers for `ietf-interfaces` model.

private:
  // internal representation (to be added)
};

} // namespace netcli
