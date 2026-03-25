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

#ifdef NC_ENABLED_SSH_TLS
#include <libnetconf2/session_client.h>
#endif

Client::Client(Session session) noexcept : session_(std::move(session)) {}

Client::~Client() = default;

std::unique_ptr<Client> Client::connect_unix(const std::string &path,
                                             const YangContext &ctx) {
  struct nc_session *s =
      nc_connect_unix(path.c_str(), const_cast<struct ly_ctx *>(ctx.get()));

  if (!s)
    return nullptr;

  Session sess(s);
  return std::unique_ptr<Client>(new Client(std::move(sess)));
}

std::unique_ptr<Client> Client::connect_inout(int fdin, int fdout,
                                              const YangContext &ctx) {
  struct nc_session *s =
      nc_connect_inout(fdin, fdout, const_cast<struct ly_ctx *>(ctx.get()));

  if (!s)
    return nullptr;

  Session sess(s);
  return std::unique_ptr<Client>(new Client(std::move(sess)));
}

#ifdef NC_ENABLED_SSH_TLS
std::unique_ptr<Client> Client::connect_ssh(const std::string &host,
                                            uint16_t port,
                                            const YangContext &ctx) {
  struct nc_session *s = nc_connect_ssh(host.c_str(), port,
                                        const_cast<struct ly_ctx *>(ctx.get()));

  if (!s)
    return nullptr;

  Session sess(s);
  return std::unique_ptr<Client>(new Client(std::move(sess)));
}

std::unique_ptr<Client> Client::connect_tls(const std::string &host,
                                            uint16_t port,
                                            const YangContext &ctx) {
  struct nc_session *s = nc_connect_tls(host.c_str(), port,
                                        const_cast<struct ly_ctx *>(ctx.get()));

  if (!s)
    return nullptr;

  Session sess(s);
  return std::unique_ptr<Client>(new Client(std::move(sess)));
}
#endif

// Singleton storage
static std::unique_ptr<Client> g_client_instance;
static std::mutex g_client_mutex;
static std::once_flag g_client_init_flag;

static void global_client_init() {
  nc_client_init();
  nc_client_set_schema_searchpath(
      "/usr/local/share/yang/modules/yang/standard/ietf/RFC");
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
  if (g_client_instance && g_client_instance->session_.getSessionPtr()) {
    nc_session_free(g_client_instance->session_.getSessionPtr(), nullptr);
  }
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
    if (op) {
      YangData d(op);

      auto r =
          std::make_unique<NetconfServerReply>(NetconfServerReply::RPL_DATA);

      r->setData(d);

      if (env)
        lyd_free_all(env);

      if (op)
        lyd_free_all(op);

      return r;
    }
    if (env)
      lyd_free_all(env);
    return std::make_unique<NetconfServerReply>(NetconfServerReply::RPL_OK);
  }

  if (env)
    lyd_free_all(env);
  if (op)
    lyd_free_all(op);
  return nullptr;
}

std::unique_ptr<NetconfServerReply>
Client::editConfig(const YangData &target [[maybe_unused]],
                   NetconfEditConfigOperation op [[maybe_unused]]) const {
  throw NotImplementedError();
}

std::unique_ptr<NetconfServerReply> Client::copyConfig(DataStore src
                                                       [[maybe_unused]],
                                                       DataStore dst
                                                       [[maybe_unused]]) const {
  throw NotImplementedError();
}

std::unique_ptr<NetconfServerReply>
Client::deleteConfig(const YangData &target [[maybe_unused]]) const {
  throw NotImplementedError();
}

std::unique_ptr<NetconfServerReply> Client::commit() const {
  throw NotImplementedError();
}

std::unique_ptr<NetconfServerReply> Client::lock(DataStore ds
                                                 [[maybe_unused]]) const {
  throw NotImplementedError();
}

std::unique_ptr<NetconfServerReply> Client::unlock(DataStore ds
                                                   [[maybe_unused]]) const {
  throw NotImplementedError();
}

std::unique_ptr<NetconfServerReply> Client::closeSession() const {
  throw NotImplementedError();
}

std::unique_ptr<NetconfServerReply>
Client::killSession(const Session &target [[maybe_unused]]) const {
  throw NotImplementedError();
}

std::unique_ptr<NetconfServerReply> Client::validate(const YangData &target
                                                     [[maybe_unused]]) const {
  throw NotImplementedError();
}

std::unique_ptr<NetconfServerReply> Client::discardChanges() const {
  throw NotImplementedError();
}
