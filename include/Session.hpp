/*
 * Session.hpp
 * Represents a NETCONF session handle
 */

#pragma once

#if !defined(STELLERI_NETCONF) || STELLERI_NETCONF != 1
#error "netconf headers are for the STELLERI_NETCONF build only"
#endif

#include <cstdint>
#include <libnetconf2/session.h>

#include "YangContext.hpp"

// libyang headers trigger -Werror for anonymous structs on some compilers;
// suppress those warnings around the include.
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-anonymous-struct"
#pragma clang diagnostic ignored "-Wnested-anon-types"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wgnu-anonymous-struct"
#pragma GCC diagnostic ignored "-Wnested-anon-types"
#endif
#include <libyang/libyang.h>
#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
#include <string>

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

  // yangContext should return YangContext.
  // Return a YangContext wrapper for the session's libyang context.
  YangContext yangContext() const {
    if (!nc_session_)
      return YangContext(nullptr);
    const struct ly_ctx *c = nc_session_get_ctx(nc_session_);
    return YangContext(c);
  }

  // Return the underlying libnetconf2 session pointer (non-owning).
  struct nc_session *getSessionPtr() const { return nc_session_; }

private:
  struct nc_session *nc_session_ = nullptr;
  mutable std::string id_;
};
