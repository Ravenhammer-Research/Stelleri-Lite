/*
 * DataStore.hpp
 * Abstract representation of a NETCONF/YANG datastore (running/candidate)
 */

#pragma once

#include <string>

class DataStore {
public:
  virtual ~DataStore() = default;

  // Human-readable datastore identifier (e.g., "running", "candidate").
  virtual std::string name() const = 0;
};
