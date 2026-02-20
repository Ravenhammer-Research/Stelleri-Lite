/*
 * YangData.hpp
 * Abstract representation of YANG-modeled data
 */

#pragma once

#include <memory>
#include <string>

#include <functional>
#include <cstdlib>

#include "YangDataTypes.hpp"
#include <libyang/libyang.h>

class YangData {
public:
  virtual ~YangData() {
    if (node_)
      lyd_free_all(node_);
  }

  // Default ctor accepts an optional libyang data tree node pointer.
  explicit YangData(struct lyd_node *node = nullptr) : node_(node) {}

  // Return a deep-cloned copy of this YangData instance. Default
  // implementation duplicates the stored libyang node and returns a
  // plain `YangData` wrapper owning the duplicated node. Subclasses
  // may override to return their concrete type.
  virtual std::unique_ptr<YangData> clone() const {
    if (!node_)
      return std::make_unique<YangData>(nullptr);
    struct lyd_node *dup = nullptr;
    LY_ERR rc = lyd_dup_single(node_, nullptr, 0, &dup);
    if (rc != LY_SUCCESS || !dup)
      return nullptr;
    return std::make_unique<YangData>(dup);
  }

  // Serialize to XML/string representation. Default implementation uses
  // libyang to print the stored `lyd_node*`.
  //
  // Deprecated: prefer working with the underlying libyang data tree via
  // `toLydNode()` and libyang printers. This method is retained for
  // compatibility but discouraged until serialization policy is clarified.
  [[deprecated("toXML is deprecated; prefer toLydNode() and letting libnetconf2 handle serialization on its own")]]
  virtual std::string toXML() const {
    if (!node_)
      return std::string();
    char *buf = nullptr;
    LY_ERR rc = lyd_print_mem(&buf, node_, LYD_XML, 0);
    if (rc != LY_SUCCESS || !buf)
      return std::string();
    std::string s(buf);
    free(buf);
    return s;
  }

  // Return the underlying libyang data tree node pointer. This does NOT
  // transfer ownership; the `YangData` instance retains ownership of the
  // node. Callers should not free the returned pointer.
  struct lyd_node *toLydNode() const { return node_; }

  // (de)serialization is NOT handled by this.

  // Factory type used by static dispatch table entries.
  using Factory = std::function<std::unique_ptr<YangData>(struct lyd_node *)>;

  // Dispatch: static table lookup similar to `InterfaceToken::dispatch`.
  // The table is intentionally left empty here; concrete entries will be
  // added later alongside concrete YangData subclasses.
  static std::unique_ptr<YangData> dispatch(YangDataType t, struct lyd_node *node) {
    struct Entry {
      YangDataType type;
      Factory factory;
    };

    static const Entry table[] = {
        // { YangDataType::IetfFoo, [](struct lyd_node *n){ return std::make_unique<MyFoo>(n); } },
    };

    for (const auto &e : table) {
      if (e.type == t)
        return e.factory(node);
    }
    return nullptr;
  }

protected:
  struct lyd_node *node_ = nullptr;
};
