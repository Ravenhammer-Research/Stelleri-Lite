/*
 * Server.hpp
 * Minimal server stubs: class declaration and rpc callback prototype.
 * No helpers, no parsing, no heavy wiring — just placeholders.
 */

#pragma once

#if !defined(STELLERI_NETCONF) || STELLERI_NETCONF != 1
#error "netconf headers are for the STELLERI_NETCONF build only"
#endif

// Forward declarations of libnetconf2/libyang types to avoid including headers
// here.
extern "C" struct nc_session;
extern "C" struct lyd_node;
extern "C" struct nc_server_reply;

class Server {
public:
  explicit Server() noexcept;
  ~Server();

  // Register/unregister the RPC callbacks with the underlying server
  // (stubbed — no actual libnetconf2 calls here).
  bool registerCallbacks();
  void unregisterCallbacks();

  // C-style RPC callback suitable for registration with libnetconf2.
  // Signature matches: (struct lyd_node *rpc, struct nc_session *session).
  static struct nc_server_reply *
  server_rpc_callback(struct lyd_node *rpc,
                      struct nc_session *session) noexcept;
};
