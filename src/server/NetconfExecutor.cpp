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

#include "NetconfExecutor.hpp"
#include "DataStore.hpp"
#include "NetconfEditConfigOperation.hpp"
#include "NetconfServerReply.hpp"
#include "Session.hpp"
#include "YangContext.hpp"
#include "YangData.hpp"
#include <cstring>
#include <libyang/libyang.h>
#include <memory>
#include <mutex>

#include "Logger.hpp"
#include "NetconfExecutor.hpp"

static struct lyd_node *g_running = nullptr;
static struct lyd_node *g_candidate = nullptr;
static std::mutex g_ds_mutex;

void NetconfExecutor::init(const struct ly_ctx *ctx) {
  auto &log = logger::get();
  std::lock_guard<std::mutex> lock(g_ds_mutex);
  if (g_running)
    return;

  log.debug("NetconfExecutor: initializing running datastore");

  // Create the root node for ietf-interfaces. Use lyd_new_path to ensure it's
  // properly initialized in the context.
  if (lyd_new_path(nullptr, ctx, "/ietf-interfaces:interfaces", nullptr, 0, &g_running) != LY_SUCCESS) {
      log.warn("NetconfExecutor: failed to create /ietf-interfaces:interfaces via lyd_new_path");
      // If ietf-interfaces is not found or fails, try to just get the module and use it.
      const struct lys_module *mod = ly_ctx_get_module_implemented(ctx, "ietf-interfaces");
      if (mod) {
          log.debug("NetconfExecutor: found ietf-interfaces module, using lyd_new_inner");
          lyd_new_inner(nullptr, mod, "interfaces", 0, &g_running);
      } else {
          log.error("NetconfExecutor: ietf-interfaces module not implemented in context!");
      }
  }

  if (g_running) {
    log.debug("NetconfExecutor: running datastore initialized successfully");
  } else {
    log.error("NetconfExecutor: failed to initialize running datastore");
  }
}

std::unique_ptr<NetconfServerReply>
NetconfExecutor::get(const Session &session, const YangData &filter) {
  return getConfig(session, filter);
}

std::unique_ptr<NetconfServerReply>
NetconfExecutor::getConfig(const Session &session,
                           const YangData &filter [[maybe_unused]]) {
  auto &log = logger::get();
  log.debug("NetconfExecutor: getConfig called");
  std::lock_guard<std::mutex> lock(g_ds_mutex);
  struct lyd_node *dup = nullptr;

  if (g_running) {
    // Use LYD_DUP_RECURSIVE (0x01) to clone the entire tree.
    if (lyd_dup_siblings(g_running, nullptr, 0x01, &dup) != LY_SUCCESS) {
      log.error("NetconfExecutor: failed to duplicate running datastore");
      auto r =
          std::make_unique<NetconfServerReply>(NetconfServerReply::RPL_ERROR);
      r->setError(NetconfServerReply::ERR_OP_FAILED, session.yangContext().get());
      return r;
    }
    log.debug("NetconfExecutor: duplicated running datastore for reply");
  } else {
    log.warn("NetconfExecutor: g_running is NULL in getConfig");
  }

  auto r = std::make_unique<NetconfServerReply>(NetconfServerReply::RPL_DATA);
  r->setData(YangData(dup));
  return r;
}

std::unique_ptr<NetconfServerReply>
NetconfExecutor::editConfig(const Session &session, const YangData &target,
                            NetconfEditConfigOperation op [[maybe_unused]]) {
  std::lock_guard<std::mutex> lock(g_ds_mutex);

  const struct lyd_node *src = target.toLydNode();
  if (!src) {
    return std::make_unique<NetconfServerReply>(NetconfServerReply::RPL_OK);
  }

  // Find the actual config nodes. If 'target' is the 'config' node of
  // edit-config, we need its children.
  const struct lyd_node *config_node = src;
  if (src->schema && src->schema->name &&
      std::strcmp(src->schema->name, "config") == 0) {
    config_node = lyd_child(src);
  }

  if (!config_node) {
    return std::make_unique<NetconfServerReply>(NetconfServerReply::RPL_OK);
  }

  // Initialize candidate with a clone of g_running if it's empty.
  if (!g_candidate) {
    if (g_running) {
      lyd_dup_siblings(g_running, nullptr, 0x01, &g_candidate);
    } else {
      const struct lys_module *mod = ly_ctx_get_module_implemented(
          session.yangContext().get(), "ietf-interfaces");
      if (mod) {
        lyd_new_inner(nullptr, mod, "interfaces", 0, &g_candidate);
      }
    }
  }

  // Apply changes to g_candidate. Merge is the default.
  if (lyd_merge_siblings(&g_candidate, config_node, 0) != LY_SUCCESS) {
    auto r =
        std::make_unique<NetconfServerReply>(NetconfServerReply::RPL_ERROR);
    r->setError(NetconfServerReply::ERR_OP_FAILED, session.yangContext().get());
    return r;
  }

  return std::make_unique<NetconfServerReply>(NetconfServerReply::RPL_OK);
}

std::unique_ptr<NetconfServerReply>
NetconfExecutor::copyConfig(const Session &session [[maybe_unused]],
                            DataStore src, DataStore dst) {
  std::lock_guard<std::mutex> lock(g_ds_mutex);

  struct lyd_node **src_ptr =
      (src == DataStore::Running) ? &g_running : &g_candidate;
  struct lyd_node **dst_ptr =
      (dst == DataStore::Running) ? &g_running : &g_candidate;

  if (*dst_ptr) {
    lyd_free_all(*dst_ptr);
    *dst_ptr = nullptr;
  }

  if (*src_ptr) {
    lyd_dup_siblings(*src_ptr, nullptr, 0x01, dst_ptr);
  }

  return std::make_unique<NetconfServerReply>(NetconfServerReply::RPL_OK);
}

std::unique_ptr<NetconfServerReply>
NetconfExecutor::deleteConfig(const Session &session [[maybe_unused]],
                              const YangData &target [[maybe_unused]]) {
  std::lock_guard<std::mutex> lock(g_ds_mutex);
  if (g_candidate) {
    lyd_free_all(g_candidate);
    g_candidate = nullptr;
  }
  return std::make_unique<NetconfServerReply>(NetconfServerReply::RPL_OK);
}

std::unique_ptr<NetconfServerReply>
NetconfExecutor::commit(const Session &session) {
  return copyConfig(session, DataStore::Candidate, DataStore::Running);
}

std::unique_ptr<NetconfServerReply> NetconfExecutor::lock(const Session &session
                                                          [[maybe_unused]],
                                                          DataStore ds
                                                          [[maybe_unused]]) {
  return std::make_unique<NetconfServerReply>(NetconfServerReply::RPL_OK);
}

std::unique_ptr<NetconfServerReply>
NetconfExecutor::unlock(const Session &session [[maybe_unused]],
                        DataStore ds [[maybe_unused]]) {
  return std::make_unique<NetconfServerReply>(NetconfServerReply::RPL_OK);
}

std::unique_ptr<NetconfServerReply>
NetconfExecutor::closeSession(const Session &session [[maybe_unused]],
                              const Session &target [[maybe_unused]]) {
  return std::make_unique<NetconfServerReply>(NetconfServerReply::RPL_OK);
}

std::unique_ptr<NetconfServerReply>
NetconfExecutor::killSession(const Session &session [[maybe_unused]],
                             const Session &target [[maybe_unused]]) {
  return std::make_unique<NetconfServerReply>(NetconfServerReply::RPL_OK);
}

std::unique_ptr<NetconfServerReply>
NetconfExecutor::validate(const Session &session [[maybe_unused]],
                          const YangData &target [[maybe_unused]]) {
  return std::make_unique<NetconfServerReply>(NetconfServerReply::RPL_OK);
}

std::unique_ptr<NetconfServerReply>
NetconfExecutor::discardChanges(const Session &session [[maybe_unused]]) {
  std::lock_guard<std::mutex> lock(g_ds_mutex);
  if (g_candidate) {
    lyd_free_all(g_candidate);
    g_candidate = nullptr;
  }
  return std::make_unique<NetconfServerReply>(NetconfServerReply::RPL_OK);
}
