/* NetconfExecutor.hpp
 * Static executor facade used by server glue. Declarations mirror
 * src/server/NetconfExecutor.cpp.
 */

#pragma once

#if !defined(STELLERI_NETCONF) || STELLERI_NETCONF != 1
#error "netconf headers are for the STELLERI_NETCONF build only"
#endif

#include "DataStore.hpp"
#include "NetconfEditConfigOperation.hpp"
#include "NetconfServerReply.hpp"
#include "Session.hpp"
#include "YangData.hpp"
#include <memory>

class NetconfExecutor {
public:
  static void init(const struct ly_ctx *ctx);

  static std::unique_ptr<NetconfServerReply> get(const Session &session,
                                                 const YangData &filter);

  static std::unique_ptr<NetconfServerReply> getConfig(const Session &session,
                                                       const YangData &filter);

  static std::unique_ptr<NetconfServerReply>
  editConfig(const Session &session, const YangData &target,
             NetconfEditConfigOperation op);

  static std::unique_ptr<NetconfServerReply>
  copyConfig(const Session &session, DataStore src, DataStore dst);

  static std::unique_ptr<NetconfServerReply>
  deleteConfig(const Session &session, const YangData &target);

  static std::unique_ptr<NetconfServerReply> commit(const Session &session);

  static std::unique_ptr<NetconfServerReply> lock(const Session &session,
                                                  DataStore ds);

  static std::unique_ptr<NetconfServerReply> unlock(const Session &session,
                                                    DataStore ds);

  static std::unique_ptr<NetconfServerReply>
  closeSession(const Session &session, const Session &target);

  static std::unique_ptr<NetconfServerReply> killSession(const Session &session,
                                                         const Session &target);

  static std::unique_ptr<NetconfServerReply> validate(const Session &session,
                                                      const YangData &target);

  static std::unique_ptr<NetconfServerReply>
  discardChanges(const Session &session);
};
