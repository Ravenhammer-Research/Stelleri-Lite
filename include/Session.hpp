/*
 * Session.hpp
 * Represents a NETCONF session handle
 */

#pragma once

#include <string>
#include <libnetconf2/session.h>
#include <libyang/libyang.h>

class Session {
public:
  Session() = default;
  explicit Session(std::string id) : id_(std::move(id)) {}
  explicit Session(struct nc_session *s) : nc_session_(s) {
    if (nc_session_)
      id_ = std::to_string(nc_session_get_id(nc_session_));
  }
  virtual ~Session() = default;

  const std::string &id() const {
    if (nc_session_)
      id_ = std::to_string(nc_session_get_id(nc_session_));
    return id_;
  }
  void setId(const std::string &id) { id_ = id; }

  // Return libyang context associated with the underlying NETCONF session.
  // Returns nullptr if no session is stored.
  const struct ly_ctx *yangContext() const { return nc_session_ ? nc_session_get_ctx(nc_session_) : nullptr; }

private:
  struct nc_session *nc_session_ = nullptr;
  mutable std::string id_;
};
