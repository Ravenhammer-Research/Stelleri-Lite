/*
 * NetconfEditConfigOperation.hpp
 * Representation of an edit-config operation for NETCONF
 */

#pragma once

#include <string>
#include <optional>

class NetconfEditConfigOperation {
public:
  enum class Operation { Merge, Replace, Create, Delete, Remove };

  NetconfEditConfigOperation() = default;
  explicit NetconfEditConfigOperation(Operation op) : op_(op) {}
  virtual ~NetconfEditConfigOperation() = default;

  Operation operation() const { return op_; }
  void setOperation(Operation o) { op_ = o; }

  // Optional target identifier (e.g., datastore path)
  const std::optional<std::string> &target() const { return target_; }
  void setTarget(const std::optional<std::string> &t) { target_ = t; }

private:
  Operation op_ = Operation::Merge;
  std::optional<std::string> target_;
};
