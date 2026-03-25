/*
 * Copyright (c) 2026, Ravenhammer Research Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#if !defined(STELLERI_NETCONF) || STELLERI_NETCONF != 1
#error "netconf headers are for the STELLERI_NETCONF build only"
#endif

#include "Server.hpp"
#include "DataStore.hpp"
#include "NetconfEditConfigOperation.hpp"
#include "NetconfExecutor.hpp"
#include "NetconfServerReply.hpp"
#include "Session.hpp"
#include "YangData.hpp"

#include <cstring>
#include <memory>

#ifndef NC_ENABLED_SSH_TLS
#define NC_ENABLED_SSH_TLS
#endif

#include <libnetconf2/messages_server.h>
#include <libnetconf2/session_server.h>
#include <libyang/libyang.h>

Server::Server() noexcept {}

Server::~Server() = default;

bool Server::registerCallbacks() {
  nc_set_global_rpc_clb(server_rpc_callback);
  return true;
}

void Server::unregisterCallbacks() { nc_set_global_rpc_clb(nullptr); }

struct nc_server_reply *
Server::server_rpc_callback(struct lyd_node *rpc,
                            struct nc_session *session) noexcept {
  Session s(session);
  std::unique_ptr<NetconfServerReply> reply;

  // Determine RPC operation name from the first child element of <rpc>.
  const struct lyd_node *op = rpc;
  const char *opname = nullptr;
  if (op) {
    const struct lysc_node *schema = op->schema;
    if (schema && schema->name)
      opname = schema->name;
  }

  // Extract the operation payload (duplicate so YangData owns it).
  struct lyd_node *data_node = nullptr;
  if (op) {
    const struct lyd_node *payload = lyd_child(op);
    if (payload) {
      (void)lyd_dup_single(payload, nullptr, 0, &data_node);
    } else {
      (void)lyd_dup_single(op, nullptr, 0, &data_node);
    }
  }
  // Construct YangData from the duplicated payload node.
  YangData data(data_node);

  if (!opname) {
    // No operation - return OK by default.
    return nc_server_reply_ok();
  }

  if (std::strcmp(opname, "get-schema") == 0) {
    return nc_clb_default_get_schema(rpc, session);
  } else if (std::strcmp(opname, "close-session") == 0) {
    return nc_clb_default_close_session(rpc, session);
  }

  if (std::strcmp(opname, "get") == 0) {
    reply = NetconfExecutor::get(s, data);
  } else if (std::strcmp(opname, "get-config") == 0) {
    reply = NetconfExecutor::getConfig(s, data);
  } else if (std::strcmp(opname, "edit-config") == 0) {
    reply =
        NetconfExecutor::editConfig(s, data, NetconfEditConfigOperation::Merge);
  } else if (std::strcmp(opname, "copy-config") == 0) {
    reply =
        NetconfExecutor::copyConfig(s, DataStore::Running, DataStore::Running);
  } else if (std::strcmp(opname, "delete-config") == 0) {
    reply = NetconfExecutor::deleteConfig(s, data);
  } else if (std::strcmp(opname, "commit") == 0) {
    reply = NetconfExecutor::commit(s);
  } else if (std::strcmp(opname, "lock") == 0) {
    reply = NetconfExecutor::lock(s, DataStore::Running);
  } else if (std::strcmp(opname, "unlock") == 0) {
    reply = NetconfExecutor::unlock(s, DataStore::Running);
  } else if (std::strcmp(opname, "kill-session") == 0) {
    reply = NetconfExecutor::killSession(s, Session());
  } else if (std::strcmp(opname, "validate") == 0) {
    reply = NetconfExecutor::validate(s, data);
  } else if (std::strcmp(opname, "discard-changes") == 0) {
    reply = NetconfExecutor::discardChanges(s);
  } else {
    // Unknown operation: return OK as a conservative default.
    return nc_server_reply_ok();
  }

  if (!reply) {
    const struct ly_ctx *ctx = nc_session_get_ctx(session);
    struct lyd_node *err = nc_err(ctx, NC_ERR_OP_FAILED, NC_ERR_TYPE_APP);
    return nc_server_reply_err(err);
  }

  return reply->toNcServerReply();
}
