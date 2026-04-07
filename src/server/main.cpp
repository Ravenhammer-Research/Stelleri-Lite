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
#error "netconf build only"
#endif

#ifndef NC_ENABLED_SSH_TLS
#define NC_ENABLED_SSH_TLS
#endif

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-anonymous-struct"
#pragma clang diagnostic ignored "-Wnested-anon-types"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wgnu-anonymous-struct"
#pragma GCC diagnostic ignored "-Wnested-anon-types"
#endif

#include <libnetconf2/log.h>
#include <libnetconf2/server_config.h>
#include <libnetconf2/session_server.h>
#include <libyang/libyang.h>

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#include "Logger.hpp"
#include "NetconfExecutor.hpp"
#include "Server.hpp"

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstring>
#include <getopt.h>
#include <string>
#include <thread>
#include <vector>

static std::atomic<bool> g_running{true};

int main(int argc, char **argv) {
  auto &log = logger::get();
  log.info("netd: starting");

  // Simple CLI: allow configuring listening endpoints.
  struct Endpoint {
    enum class Type { Unix, SSH, TLS, Tcp } type;
    std::string addr;
    uint16_t port{};
    std::string extra; // for unix path or other params
  };

  std::vector<Endpoint> endpoints;

  // Default unix socket path when none is specified (netconf build only)
  const std::string default_unix_socket = "netconf.sock";

  const char *short_opts = "hv";
  const struct option long_opts[] = {{"help", no_argument, nullptr, 'h'},
                                     {"verbose", no_argument, nullptr, 'v'},
                                     {"unix", required_argument, nullptr, 0},
                                     {"tcp", required_argument, nullptr, 0},
                                     {"ssh", required_argument, nullptr, 0},
                                     {"tls", required_argument, nullptr, 0},
                                     {nullptr, 0, nullptr, 0}};

  int idx = 0;
  int c;

  while ((c = getopt_long(argc, argv, short_opts, long_opts, &idx)) != -1) {
    if (c == 'h') {
      std::puts("Usage: netd [-v] [--unix PATH] [--tcp ADDR:PORT] [--ssh ADDR:PORT] "
                "[--tls ADDR:PORT]");
      return 0;
    } else if (c == 'v') {
      log.setLevel(logger::Level::Debug);
    } else if (c == 0) {
      const char *name = long_opts[idx].name;
      if (std::strcmp(name, "unix") == 0) {
        Endpoint e;
        e.type = Endpoint::Type::Unix;
        e.extra = optarg;
        endpoints.push_back(std::move(e));
      } else if (std::strcmp(name, "tcp") == 0) {
        // format address:port or port
        Endpoint e;
        e.type = Endpoint::Type::Tcp;
        std::string arg(optarg);
        auto p = arg.find(':');

        if (p == std::string::npos) {
          e.addr = "0.0.0.0";
          e.port = static_cast<uint16_t>(std::stoi(arg));
        } else {
          e.addr = arg.substr(0, p);
          e.port = static_cast<uint16_t>(std::stoi(arg.substr(p + 1)));
        }
        endpoints.push_back(std::move(e));
      } else if (std::strcmp(name, "ssh") == 0) {
        Endpoint e;
        e.type = Endpoint::Type::SSH;
        std::string arg(optarg);
        auto p = arg.find(':');

        if (p == std::string::npos) {
          e.addr = "0.0.0.0";
          e.port = NC_PORT_SSH;
        } else {
          e.addr = arg.substr(0, p);
          e.port = static_cast<uint16_t>(std::stoi(arg.substr(p + 1)));
        }
        endpoints.push_back(std::move(e));
      } else if (std::strcmp(name, "tls") == 0) {
        Endpoint e;
        e.type = Endpoint::Type::TLS;
        std::string arg(optarg);
        auto p = arg.find(':');

        if (p == std::string::npos) {
          e.addr = "0.0.0.0";
          e.port = NC_PORT_TLS;
        } else {
          e.addr = arg.substr(0, p);
          e.port = static_cast<uint16_t>(std::stoi(arg.substr(p + 1)));
        }
        endpoints.push_back(std::move(e));
      }
    }
  }

  if (nc_server_init() != 0) {
    log.error("netd: nc_server_init() failed");
    return 1;
  }

  // Set base directory for UNIX sockets.
  if (nc_server_set_unix_socket_dir("/var/run/stelleri") != 0) {
    log.warn("netd: failed to set unix socket base directory");
  }

  // Initialize or obtain a libyang context for the server.
  struct ly_ctx *ctx = nullptr;

  if (nc_server_init_ctx(&ctx) != 0) {
    log.warn("netd: nc_server_init_ctx() reported an issue (continuing)");
  }

  // Load and implement ietf-interfaces so clients can use it.
  if (ctx) {
    ly_ctx_set_searchdir(ctx, "/usr/local/share/yang/modules/yang/standard/ietf/RFC");
    ly_ctx_set_searchdir(ctx, "/usr/local/share/yang/modules/yang/standard/iana");
    ly_ctx_set_searchdir(ctx, "/usr/local/share/yang/modules/libnetconf2");
    const struct lys_module *mod =
        ly_ctx_load_module(ctx, "ietf-interfaces", nullptr, nullptr);

    if (mod) {
      if (lys_set_implemented(const_cast<struct lys_module *>(mod), nullptr) !=
          LY_SUCCESS) {
        log.warn("netd: failed to implement ietf-interfaces");
      }
    } else {
      log.warn("netd: failed to load ietf-interfaces");
    }

    // Enable NETCONF capabilities by setting features on ietf-netconf.
    struct lys_module *nc_mod =
        ly_ctx_get_module_implemented(ctx, "ietf-netconf");
    if (nc_mod) {
      const char *nc_features[] = {"candidate", "writable-running",
                                   "rollback-on-error", "validate", "xpath",
                                   nullptr};
      if (lys_set_implemented(nc_mod, nc_features) != LY_SUCCESS) {
        log.warn("netd: failed to enable ietf-netconf features");
      }
    } else {
      log.warn("netd: ietf-netconf module not found in context");
    }

    NetconfExecutor::init(ctx);
  }

  // If endpoints were requested on the CLI, build server configuration and
  // apply it.
  if (endpoints.empty()) {
    Endpoint e;
    e.type = Endpoint::Type::Unix;
    e.extra = default_unix_socket;
    endpoints.push_back(std::move(e));
  }

  if (!endpoints.empty()) {
    if (nc_server_config_load_modules(&ctx) != 0) {
      log.error("netd: failed to load server config modules");
    } else {
      // Enable 'unix-socket-path' feature for 'libnetconf2-netconf-server'.
      struct lys_module *mod =
          ly_ctx_get_module_implemented(ctx, "libnetconf2-netconf-server");

      if (mod) {
        const char *features[] = {"unix-socket-path", nullptr};
        if (lys_set_implemented(mod, features) != LY_SUCCESS) {
          log.warn("netd: failed to enable 'unix-socket-path' feature for "
                   "'libnetconf2-netconf-server'");
        }
      }

      struct lyd_node *config = nullptr;
      unsigned count = 0;

      for (const auto &ep : endpoints) {
        std::string name = "ep" + std::to_string(count++);
        if (ep.type == Endpoint::Type::Unix) {
          if (nc_server_config_add_unix_socket(
                  ctx, name.c_str(), ep.extra.c_str(), nullptr, nullptr,
                  nullptr, &config) != 0) {
            log.warn("netd: failed to add unix socket endpoint");
          }
        } else {
#ifdef NC_ENABLED_SSH_TLS
          // For SSH/TLS/TCP use address/port. Map TCP->TLS/SSH transport where
          // applicable.
          NC_TRANSPORT_IMPL ti =
              (ep.type == Endpoint::Type::SSH) ? NC_TI_SSH : NC_TI_TLS;

          if (ep.type == Endpoint::Type::Tcp)
            ti = NC_TI_TLS; // TCP treated as TLS by default

          if (nc_server_config_add_address_port(ctx, name.c_str(), ti,
                                                ep.addr.c_str(), ep.port,
                                                &config) != 0) {
            log.warn("netd: failed to add address/port endpoint");
          }
#else
          log.warn("netd: SSH/TLS endpoints not supported by libnetconf2 "
                   "build; skipping");
#endif
        }
      }

      if (config) {
        if (nc_server_config_setup_data(config) != 0) {
          log.warn("netd: applying server configuration failed");
        }
      }
    }
  }

  Server srv;
  if (!srv.registerCallbacks()) {
    log.warn("netd: Server::registerCallbacks() failed (continuing)");
  }

  // Install signal handlers
  std::signal(SIGINT, [](int) { g_running.store(false); });
  std::signal(SIGTERM, [](int) { g_running.store(false); });

  struct nc_pollsession *ps = nc_ps_new();
  if (!ps) {
    log.error("netd: failed to create pollsession structure");
    return 1;
  }

  // Main loop
  while (g_running.load()) {
    struct nc_session *new_sess = nullptr;

    // Non-blocking accept for new sessions
    NC_MSG_TYPE msgtype = nc_accept(100, ctx, &new_sess);

    if (msgtype == NC_MSG_HELLO) {
      if (nc_ps_add_session(ps, new_sess) != 0) {
        log.warn("netd: failed to add new session to pollsession");
        nc_session_free(new_sess, nullptr);
      }
    }

    // Process existing sessions
    struct nc_session *active_sess = nullptr;
    int ret = nc_ps_poll(ps, 100, &active_sess);

    if (ret & (NC_PSPOLL_SESSION_TERM | NC_PSPOLL_SESSION_ERROR)) {
      if (active_sess) {
        nc_ps_del_session(ps, active_sess);
        nc_session_free(active_sess, nullptr);
      }
    }
  }

  log.info("netd: shutting down");
  nc_ps_free(ps);
  srv.unregisterCallbacks();
  nc_server_destroy();
  return 0;
}
