/*
 * NetconfServerReply.hpp
 * Concrete wrapper for libnetconf2 server rpc-reply objects
 */

#pragma once

#if !defined(STELLERI_NETCONF) || STELLERI_NETCONF != 1
#error "netconf headers are for the STELLERI_NETCONF build only"
#endif

#include "YangData.hpp"
#include <libnetconf2/messages_server.h>
#include <libnetconf2/netconf.h>

enum WdMode {
  WD_UNKNOWN = NC_WD_UNKNOWN,
  WD_ALL = NC_WD_ALL,
  WD_ALL_TAG = NC_WD_ALL_TAG,
  WD_TRIM = NC_WD_TRIM,
  WD_EXPLICIT = NC_WD_EXPLICIT
};

enum ParamType {
  PARAMTYPE_CONST = NC_PARAMTYPE_CONST,
  PARAMTYPE_FREE = NC_PARAMTYPE_FREE,
  PARAMTYPE_DUP_AND_FREE = NC_PARAMTYPE_DUP_AND_FREE
};

class NetconfServerReply {
public:
  // Local enum mapping of libnetconf2 reply types.
  enum ReplyType {
    RPL_OK = NC_RPL_OK,
    RPL_DATA = NC_RPL_DATA,
    RPL_ERROR = NC_RPL_ERROR,
    RPL_NOTIF = NC_RPL_NOTIF
  };

  enum ErrorCode {
    ERR_IN_USE = NC_ERR_IN_USE,
    ERR_INVALID_VALUE = NC_ERR_INVALID_VALUE,
    ERR_ACCESS_DENIED = NC_ERR_ACCESS_DENIED,
    ERR_ROLLBACK_FAILED = NC_ERR_ROLLBACK_FAILED,
    ERR_OP_NOT_SUPPORTED = NC_ERR_OP_NOT_SUPPORTED,
    ERR_TOO_BIG = NC_ERR_TOO_BIG,
    ERR_RES_DENIED = NC_ERR_RES_DENIED,
    ERR_OP_FAILED = NC_ERR_OP_FAILED,
    ERR_MISSING_ATTR = NC_ERR_MISSING_ATTR,
    ERR_BAD_ATTR = NC_ERR_BAD_ATTR,
    ERR_UNKNOWN_ATTR = NC_ERR_UNKNOWN_ATTR,
    ERR_MISSING_ELEM = NC_ERR_MISSING_ELEM,
    ERR_BAD_ELEM = NC_ERR_BAD_ELEM,
    ERR_UNKNOWN_ELEM = NC_ERR_UNKNOWN_ELEM,
    ERR_UNKNOWN_NS = NC_ERR_UNKNOWN_NS,
    ERR_LOCK_DENIED = NC_ERR_LOCK_DENIED,
    ERR_DATA_EXISTS = NC_ERR_DATA_EXISTS,
    ERR_DATA_MISSING = NC_ERR_DATA_MISSING,
    ERR_MALFORMED_MSG = NC_ERR_MALFORMED_MSG
  };
  explicit NetconfServerReply(ReplyType r) : rpl_(r) {}
  ~NetconfServerReply() = default;

  void setData(const YangData &data) {
    data_.reset();
    auto d = data.clone();
    if (!d)
      return;
    data_ = std::move(d);
    rpl_ = RPL_DATA;
  }

  void setError(ErrorCode code, const struct ly_ctx *ctx = nullptr,
                NC_ERR_TYPE type = NC_ERR_TYPE_APP) {
    rpl_ = RPL_ERROR;
    NC_ERR tag = static_cast<NC_ERR>(code);
    switch (tag) {
    case NC_ERR_IN_USE:
    case NC_ERR_INVALID_VALUE:
    case NC_ERR_ACCESS_DENIED:
    case NC_ERR_ROLLBACK_FAILED:
    case NC_ERR_OP_NOT_SUPPORTED:
    case NC_ERR_TOO_BIG:
    case NC_ERR_RES_DENIED:
    case NC_ERR_OP_FAILED:
      err_ = nc_err(ctx, tag, type);
      break;
    case NC_ERR_DATA_EXISTS:
    case NC_ERR_DATA_MISSING:
    case NC_ERR_MALFORMED_MSG:
      err_ = nc_err(ctx, tag);
      break;
    default:
      // Other tags (like MISSING_ATTR) require more arguments than just 'type'.
      // For a conservative default that avoids 'Invalid argument' errors,
      // pass 'type' and hope for the best, or use OP_FAILED if context is
      // missing.
      err_ = nc_err(ctx, tag, type);
      break;
    }
  }
  // If you have a libyang context and want to convert libyang diagnostics to
  // an <rpc-error>, build it via nc_err() when available. The helper
  // nc_err_libyang() is not available in all libnetconf2 versions, so it is
  // not used here.

  struct nc_server_reply *toNcServerReply(WdMode wd = WD_UNKNOWN,
                                          ParamType pt = PARAMTYPE_DUP_AND_FREE) const {
    if (rpl_ == RPL_OK)
      return nc_server_reply_ok();

    if (rpl_ == RPL_DATA) {
      struct lyd_node *src = data_ ? data_->toLydNode() : nullptr;
      if (!src)
        return nc_server_reply_ok();
      return nc_server_reply_data(src, static_cast<NC_WD_MODE>(wd),
                                  static_cast<NC_PARAMTYPE>(pt));
    }

    if (rpl_ == RPL_ERROR) {
      if (err_)
        return nc_server_reply_err(err_);
    }

    return nullptr;
  }

  bool isError() const { return rpl_ == RPL_ERROR; }
  bool isOk() const { return rpl_ == RPL_OK; }
  bool hasData() const { return rpl_ == RPL_DATA; }
  bool isNotification() const { return rpl_ == RPL_NOTIF; }

  // Return a cloned YangData wrapper of the stored reply data. Caller
  // receives ownership and may inspect or traverse the libyang nodes.
  std::unique_ptr<YangData> getData() const {
    if (!data_)
      return nullptr;
    return data_->clone();
  }

private:
  ReplyType rpl_;
  std::unique_ptr<YangData> data_;
  struct lyd_node *err_ = nullptr;
};
