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
#error "netconf sources are for the STELLERI_NETCONF build only"
#endif

#include "Client.hpp"

#include "DataStore.hpp"
#include "NetconfEditConfigOperation.hpp"
#include "NetconfServerReply.hpp"
#include "NotImplementedError.hpp"
#include "YangData.hpp"
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <libnetconf2/log.h>
#include <libnetconf2/messages_client.h>
#include <libnetconf2/session.h>
#include <libnetconf2/session_client.h>
#include <libyang/libyang.h>

// libnetconf2 headers may include libyang headers that trigger -Werror on
// anonymous/gnu extensions; silence those diagnostics locally.
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-anonymous-struct"
#pragma clang diagnostic ignored "-Wnested-anon-types"
#pragma clang diagnostic ignored "-Wunused-parameter"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wgnu-anonymous-struct"
#pragma GCC diagnostic ignored "-Wnested-anon-types"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif
#include <libnetconf2/session.h>
#include <libnetconf2/session_client.h>
#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#include <cstdlib>
#include <mutex>
#include "Logger.hpp"

static std::once_flag g_client_init_flag;
static std::mutex g_client_mutex;
static std::unique_ptr<Client> g_client_instance;

// Forward libnetconf2 print messages into our logger.
static void nc_print_clb(const struct nc_session * /*session*/,
                         NC_VERB_LEVEL level, const char *msg) {
  using logger::Level;
  auto &log = logger::get();
  std::string m = "libnetconf2: ";
  m += (msg ? msg : "");

  switch (level) {
  case NC_VERB_ERROR:
    log.error(m);
    break;
  case NC_VERB_WARNING:
    log.warn(m);
    break;
  case NC_VERB_VERBOSE:
    log.info(m);
    break;
  case NC_VERB_DEBUG:
  case NC_VERB_DEBUG_LOWLVL:
    log.debug(m);
    break;
  default:
    log.info(m);
    break;
  }
}

// Forward libyang print messages into our logger.
static void ly_print_clb(LY_LOG_LEVEL level, const char *msg,
                         const char * /*data_path*/, const char * /*schema_path*/,
                         uint64_t /*line*/) {
  using logger::Level;
  auto &log = logger::get();
  std::string m = "libyang: ";
  m += (msg ? msg : "");

  switch (level) {
  case LY_LLERR:
    log.error(m);
    break;
  case LY_LLWRN:
    log.warn(m);
    break;
  case LY_LLVRB:
    log.info(m);
    break;
  case LY_LLDBG:
    log.debug(m);
    break;
  default:
    log.info(m);
    break;
  }
}

static void global_client_init() {
  nc_client_init();
  nc_set_print_clb_session(nc_print_clb);
  ly_set_log_clb(ly_print_clb);

  // Set default verbosity for client
  nc_verbosity(NC_VERB_WARNING);
  ly_log_level(LY_LLWRN);

  nc_client_set_schema_searchpath("/usr/local/share/yang/modules/yang/standard/ietf/RFC");
}

Client::Client(Session session) noexcept : session_(std::move(session)) {}

Client::~Client() {
  if (session_.getSessionPtr()) {
    nc_session_free(session_.getSessionPtr(), nullptr);
  }
}

std::unique_ptr<Client> Client::connect_unix(const std::string &path,
                                             const YangContext &ctx) {
  struct ly_ctx *ly_ctx = const_cast<struct ly_ctx *>(ctx.get());
  bool own_ctx = false;

  if (!ly_ctx) {
    if (ly_ctx_new(nullptr, LY_CTX_DISABLE_SEARCHDIRS, &ly_ctx) != LY_SUCCESS) {
      return nullptr;
    }
    ly_ctx_set_searchdir(ly_ctx, "/usr/local/share/yang/modules/libnetconf2");
    ly_ctx_set_searchdir(ly_ctx, "/usr/local/share/yang/modules/yang/standard/ietf/RFC");
    ly_ctx_set_searchdir(ly_ctx, "/usr/local/share/yang/modules/yang/standard/iana");
    own_ctx = true;
  }

  struct nc_session *s = nc_connect_unix(path.c_str(), ly_ctx);
  if (!s) {
    if (own_ctx) {
      ly_ctx_destroy(ly_ctx);
    }
    return nullptr;
  }
  return std::make_unique<Client>(Session(s));
}

std::unique_ptr<Client> Client::connect_inout(int fdin, int fdout,
                                              const YangContext &ctx) {
  struct ly_ctx *ly_ctx = const_cast<struct ly_ctx *>(ctx.get());
  bool own_ctx = false;

  if (!ly_ctx) {
    if (ly_ctx_new(nullptr, LY_CTX_DISABLE_SEARCHDIRS, &ly_ctx) != LY_SUCCESS) {
      return nullptr;
    }
    ly_ctx_set_searchdir(ly_ctx, "/usr/local/share/yang/modules/libnetconf2");
    ly_ctx_set_searchdir(ly_ctx, "/usr/local/share/yang/modules/yang/standard/ietf/RFC");
    ly_ctx_set_searchdir(ly_ctx, "/usr/local/share/yang/modules/yang/standard/iana");
    own_ctx = true;
  }

  struct nc_session *s = nc_connect_inout(fdin, fdout, ly_ctx);
  if (!s) {
    if (own_ctx) {
      ly_ctx_destroy(ly_ctx);
    }
    return nullptr;
  }
  return std::make_unique<Client>(Session(s));
}
bool Client::init_unix(const std::string &path, const YangContext &ctx) {
  std::call_once(g_client_init_flag, global_client_init);
  auto tmp = Client::connect_unix(path, ctx);

  if (!tmp)
    return false;

  std::lock_guard<std::mutex> lk(g_client_mutex);
  g_client_instance = std::move(tmp);
  return true;
}

bool Client::init_inout(int fdin, int fdout, const YangContext &ctx) {
  std::call_once(g_client_init_flag, global_client_init);
  auto tmp = Client::connect_inout(fdin, fdout, ctx);

  if (!tmp)
    return false;

  std::lock_guard<std::mutex> lk(g_client_mutex);
  g_client_instance = std::move(tmp);
  return true;
}

#ifdef NC_ENABLED_SSH_TLS
std::unique_ptr<Client> Client::connect_ssh(const std::string &host,
                                            uint16_t port,
                                            const YangContext &ctx) {
  // SSH connection setup requires more configuration (auth, etc)
  // this is a placeholder or should be implemented if needed.
  throw NotImplementedError();
}

std::unique_ptr<Client> Client::connect_tls(const std::string &host,
                                            uint16_t port,
                                            const YangContext &ctx) {
  throw NotImplementedError();
}

bool Client::init_ssh(const std::string &host, uint16_t port,
                      const YangContext &ctx) {
  std::call_once(g_client_init_flag, global_client_init);
  auto tmp = Client::connect_ssh(host, port, ctx);

  if (!tmp)
    return false;

  std::lock_guard<std::mutex> lk(g_client_mutex);
  g_client_instance = std::move(tmp);
  return true;
}

bool Client::init_tls(const std::string &host, uint16_t port,
                      const YangContext &ctx) {
  std::call_once(g_client_init_flag, global_client_init);
  auto tmp = Client::connect_tls(host, port, ctx);

  if (!tmp)
    return false;

  std::lock_guard<std::mutex> lk(g_client_mutex);
  g_client_instance = std::move(tmp);
  return true;
}
#endif

Client *Client::instance() noexcept {
  std::lock_guard<std::mutex> lk(g_client_mutex);
  return g_client_instance.get();
}

void Client::shutdown() noexcept {
  std::lock_guard<std::mutex> lk(g_client_mutex);
  g_client_instance.reset();
}

// Helper removed per request; RPC methods inline send/receive logic below.

std::unique_ptr<NetconfServerReply> Client::get(const YangData &filter
                                                [[maybe_unused]]) const {
  throw NotImplementedError();
}

std::unique_ptr<NetconfServerReply> Client::getConfig(const YangData &filter
                                                      [[maybe_unused]]) const {
  // Always request the full running datastore; client-side filtering
  // avoids any XML serialization and matches the installed libnetconf2 API.
  struct nc_rpc *rpc = nc_rpc_getconfig(NC_DATASTORE_RUNNING, nullptr,
                                        NC_WD_UNKNOWN, NC_PARAMTYPE_CONST);

  if (!rpc)
    return nullptr;
  uint64_t msgid = 0;
  NC_MSG_TYPE mt = nc_send_rpc(session_.getSessionPtr(), rpc, -1, &msgid);

  if (mt != NC_MSG_RPC) {
    nc_rpc_free(rpc);
    return nullptr;
  }

  struct lyd_node *env = nullptr;
  struct lyd_node *op = nullptr;

  NC_MSG_TYPE rt =
      nc_recv_reply(session_.getSessionPtr(), rpc, msgid, -1, &env, &op);

  nc_rpc_free(rpc);

  if (rt == NC_MSG_REPLY) {
    auto r =
        std::make_unique<NetconfServerReply>(NetconfServerReply::RPL_DATA);
    if (op) {
      YangData d(op);
      r->setData(d);
    }

    if (env)
      lyd_free_all(env);

    if (op)
      lyd_free_all(op);

    return r;
  }

  if (env)
    lyd_free_all(env);
  if (op)
    lyd_free_all(op);
  return nullptr;
}

std::unique_ptr<NetconfServerReply>
Client::editConfig(const YangData &target,
                   NetconfEditConfigOperation op [[maybe_unused]]) const {
  const struct lyd_node *config_node = target.toLydNode();
  if (!config_node)
    return nullptr;

  // Serialize config_node to XML string.
  char *xml = nullptr;
  if (lyd_print_mem(&xml, config_node, LYD_XML, 0) != LY_SUCCESS) {
    return nullptr;
  }

  // Prefer candidate datastore if supported by the server.
  NC_DATASTORE ds = NC_DATASTORE_RUNNING;
  if (nc_session_cpblt(session_.getSessionPtr(),
                       "urn:ietf:params:netconf:capability:candidate:1.0")) {
    ds = NC_DATASTORE_CANDIDATE;
  }

  struct nc_rpc *rpc =
      nc_rpc_edit(ds, NC_RPC_EDIT_DFLTOP_MERGE, NC_RPC_EDIT_TESTOPT_SET,
                  NC_RPC_EDIT_ERROPT_STOP, xml, NC_PARAMTYPE_FREE);

  if (!rpc) {
    free(xml);
    return nullptr;
  }

  uint64_t msgid = 0;
  NC_MSG_TYPE mt = nc_send_rpc(session_.getSessionPtr(), rpc, -1, &msgid);
  if (mt != NC_MSG_RPC) {
    nc_rpc_free(rpc);
    return nullptr;
  }

  struct lyd_node *env = nullptr;
  struct lyd_node *op_res = nullptr;
  NC_MSG_TYPE rt =
      nc_recv_reply(session_.getSessionPtr(), rpc, msgid, -1, &env, &op_res);

  nc_rpc_free(rpc);

  if (rt == NC_MSG_REPLY) {
    auto r = std::make_unique<NetconfServerReply>(NetconfServerReply::RPL_OK);
    if (env)
      lyd_free_all(env);
    if (op_res)
      lyd_free_all(op_res);
    return r;
  }

  if (env)
    lyd_free_all(env);
  if (op_res)
    lyd_free_all(op_res);
  return nullptr;
}

std::unique_ptr<NetconfServerReply> Client::copyConfig(DataStore src,
                                                       DataStore dst) const {
  auto to_nc_ds = [](DataStore d) {
    return (d == DataStore::Running) ? NC_DATASTORE_RUNNING
                                     : NC_DATASTORE_CANDIDATE;
  };

  struct nc_rpc *rpc =
      nc_rpc_copy(to_nc_ds(dst), nullptr, to_nc_ds(src), nullptr, NC_WD_UNKNOWN,
                  NC_PARAMTYPE_CONST);

  if (!rpc)
    return nullptr;

  uint64_t msgid = 0;
  NC_MSG_TYPE mt = nc_send_rpc(session_.getSessionPtr(), rpc, -1, &msgid);
  if (mt != NC_MSG_RPC) {
    nc_rpc_free(rpc);
    return nullptr;
  }

  struct lyd_node *env = nullptr;
  struct lyd_node *op = nullptr;
  NC_MSG_TYPE rt =
      nc_recv_reply(session_.getSessionPtr(), rpc, msgid, -1, &env, &op);

  nc_rpc_free(rpc);

  if (rt == NC_MSG_REPLY) {
    auto r = std::make_unique<NetconfServerReply>(NetconfServerReply::RPL_OK);
    if (env)
      lyd_free_all(env);
    if (op)
      lyd_free_all(op);
    return r;
  }

  if (env)
    lyd_free_all(env);
  if (op)
    lyd_free_all(op);
  return nullptr;
}

std::unique_ptr<NetconfServerReply>
Client::deleteConfig(const YangData &target [[maybe_unused]]) const {
  // Simplistic delete-config for candidate.
  struct nc_rpc *rpc = nc_rpc_delete(NC_DATASTORE_CANDIDATE, nullptr,
                                     NC_PARAMTYPE_CONST);

  if (!rpc)
    return nullptr;

  uint64_t msgid = 0;
  NC_MSG_TYPE mt = nc_send_rpc(session_.getSessionPtr(), rpc, -1, &msgid);
  if (mt != NC_MSG_RPC) {
    nc_rpc_free(rpc);
    return nullptr;
  }

  struct lyd_node *env = nullptr;
  struct lyd_node *op = nullptr;
  NC_MSG_TYPE rt =
      nc_recv_reply(session_.getSessionPtr(), rpc, msgid, -1, &env, &op);

  nc_rpc_free(rpc);

  if (rt == NC_MSG_REPLY) {
    auto r = std::make_unique<NetconfServerReply>(NetconfServerReply::RPL_OK);
    if (env)
      lyd_free_all(env);
    if (op)
      lyd_free_all(op);
    return r;
  }

  if (env)
    lyd_free_all(env);
  if (op)
    lyd_free_all(op);
  return nullptr;
}

std::unique_ptr<NetconfServerReply> Client::commit() const {
  struct nc_rpc *rpc = nc_rpc_commit(0, 0, nullptr, nullptr, NC_PARAMTYPE_CONST);

  if (!rpc)
    return nullptr;

  uint64_t msgid = 0;
  NC_MSG_TYPE mt = nc_send_rpc(session_.getSessionPtr(), rpc, -1, &msgid);
  if (mt != NC_MSG_RPC) {
    nc_rpc_free(rpc);
    return nullptr;
  }

  struct lyd_node *env = nullptr;
  struct lyd_node *op = nullptr;
  NC_MSG_TYPE rt =
      nc_recv_reply(session_.getSessionPtr(), rpc, msgid, -1, &env, &op);

  nc_rpc_free(rpc);

  if (rt == NC_MSG_REPLY) {
    auto r = std::make_unique<NetconfServerReply>(NetconfServerReply::RPL_OK);
    if (env)
      lyd_free_all(env);
    if (op)
      lyd_free_all(op);
    return r;
  }

  if (env)
    lyd_free_all(env);
  if (op)
    lyd_free_all(op);
  return nullptr;
}

std::unique_ptr<NetconfServerReply> Client::lock(DataStore ds) const {
  NC_DATASTORE ncds = (ds == DataStore::Running) ? NC_DATASTORE_RUNNING
                                                 : NC_DATASTORE_CANDIDATE;
  struct nc_rpc *rpc = nc_rpc_lock(ncds);

  if (!rpc)
    return nullptr;

  uint64_t msgid = 0;
  NC_MSG_TYPE mt = nc_send_rpc(session_.getSessionPtr(), rpc, -1, &msgid);
  if (mt != NC_MSG_RPC) {
    nc_rpc_free(rpc);
    return nullptr;
  }

  struct lyd_node *env = nullptr;
  struct lyd_node *op = nullptr;
  NC_MSG_TYPE rt =
      nc_recv_reply(session_.getSessionPtr(), rpc, msgid, -1, &env, &op);

  nc_rpc_free(rpc);

  if (rt == NC_MSG_REPLY) {
    auto r = std::make_unique<NetconfServerReply>(NetconfServerReply::RPL_OK);
    if (env)
      lyd_free_all(env);
    if (op)
      lyd_free_all(op);
    return r;
  }

  if (env)
    lyd_free_all(env);
  if (op)
    lyd_free_all(op);
  return nullptr;
}

std::unique_ptr<NetconfServerReply> Client::unlock(DataStore ds) const {
  NC_DATASTORE ncds = (ds == DataStore::Running) ? NC_DATASTORE_RUNNING
                                                 : NC_DATASTORE_CANDIDATE;
  struct nc_rpc *rpc = nc_rpc_unlock(ncds);

  if (!rpc)
    return nullptr;

  uint64_t msgid = 0;
  NC_MSG_TYPE mt = nc_send_rpc(session_.getSessionPtr(), rpc, -1, &msgid);
  if (mt != NC_MSG_RPC) {
    nc_rpc_free(rpc);
    return nullptr;
  }

  struct lyd_node *env = nullptr;
  struct lyd_node *op = nullptr;
  NC_MSG_TYPE rt =
      nc_recv_reply(session_.getSessionPtr(), rpc, msgid, -1, &env, &op);

  nc_rpc_free(rpc);

  if (rt == NC_MSG_REPLY) {
    auto r = std::make_unique<NetconfServerReply>(NetconfServerReply::RPL_OK);
    if (env)
      lyd_free_all(env);
    if (op)
      lyd_free_all(op);
    return r;
  }

  if (env)
    lyd_free_all(env);
  if (op)
    lyd_free_all(op);
  return nullptr;
}

std::unique_ptr<NetconfServerReply> Client::closeSession() const {
  // <close-session> is done implicitly by freeing the session.
  return std::make_unique<NetconfServerReply>(NetconfServerReply::RPL_OK);
}

std::unique_ptr<NetconfServerReply>
Client::killSession(const Session &target) const {
  struct nc_rpc *rpc = nc_rpc_kill(nc_session_get_id(target.getSessionPtr()));

  if (!rpc)
    return nullptr;

  uint64_t msgid = 0;
  NC_MSG_TYPE mt = nc_send_rpc(session_.getSessionPtr(), rpc, -1, &msgid);
  if (mt != NC_MSG_RPC) {
    nc_rpc_free(rpc);
    return nullptr;
  }

  struct lyd_node *env = nullptr;
  struct lyd_node *op = nullptr;
  NC_MSG_TYPE rt =
      nc_recv_reply(session_.getSessionPtr(), rpc, msgid, -1, &env, &op);

  nc_rpc_free(rpc);

  if (rt == NC_MSG_REPLY) {
    auto r = std::make_unique<NetconfServerReply>(NetconfServerReply::RPL_OK);
    if (env)
      lyd_free_all(env);
    if (op)
      lyd_free_all(op);
    return r;
  }

  if (env)
    lyd_free_all(env);
  if (op)
    lyd_free_all(op);
  return nullptr;
}

std::unique_ptr<NetconfServerReply>
Client::validate(const YangData &target) const {
  const struct lyd_node *node = target.toLydNode();
  char *xml = nullptr;
  if (lyd_print_mem(&xml, node, LYD_XML, 0) != LY_SUCCESS) {
    return nullptr;
  }

  struct nc_rpc *rpc =
      nc_rpc_validate(NC_DATASTORE_CANDIDATE, xml, NC_PARAMTYPE_FREE);

  if (!rpc) {
    free(xml);
    return nullptr;
  }

  uint64_t msgid = 0;
  NC_MSG_TYPE mt = nc_send_rpc(session_.getSessionPtr(), rpc, -1, &msgid);
  if (mt != NC_MSG_RPC) {
    nc_rpc_free(rpc);
    return nullptr;
  }

  struct lyd_node *env = nullptr;
  struct lyd_node *op = nullptr;
  NC_MSG_TYPE rt =
      nc_recv_reply(session_.getSessionPtr(), rpc, msgid, -1, &env, &op);

  nc_rpc_free(rpc);

  if (rt == NC_MSG_REPLY) {
    auto r = std::make_unique<NetconfServerReply>(NetconfServerReply::RPL_OK);
    if (env)
      lyd_free_all(env);
    if (op)
      lyd_free_all(op);
    return r;
  }

  if (env)
    lyd_free_all(env);
  if (op)
    lyd_free_all(op);
  return nullptr;
}

std::unique_ptr<NetconfServerReply> Client::discardChanges() const {
  struct nc_rpc *rpc = nc_rpc_discard();

  if (!rpc)
    return nullptr;

  uint64_t msgid = 0;
  NC_MSG_TYPE mt = nc_send_rpc(session_.getSessionPtr(), rpc, -1, &msgid);
  if (mt != NC_MSG_RPC) {
    nc_rpc_free(rpc);
    return nullptr;
  }

  struct lyd_node *env = nullptr;
  struct lyd_node *op = nullptr;
  NC_MSG_TYPE rt =
      nc_recv_reply(session_.getSessionPtr(), rpc, msgid, -1, &env, &op);

  nc_rpc_free(rpc);

  if (rt == NC_MSG_REPLY) {
    auto r = std::make_unique<NetconfServerReply>(NetconfServerReply::RPL_OK);
    if (env)
      lyd_free_all(env);
    if (op)
      lyd_free_all(op);
    return r;
  }

  if (env)
    lyd_free_all(env);
  if (op)
    lyd_free_all(op);
  return nullptr;
}
