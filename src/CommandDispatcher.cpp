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

#include "CommandDispatcher.hpp"
#include "ArpToken.hpp"
#include "DeleteToken.hpp"
#include "InterfaceToken.hpp"
#include "NdpToken.hpp"
#include "RouteToken.hpp"
#include "SetToken.hpp"
#include "ShowToken.hpp"
#include "VRFToken.hpp"

namespace netcli {

  // Forward declarations for the free-function handlers implemented in
  // the individual Execute*.cpp files.
  void executeShowInterface(const InterfaceToken &, ConfigurationManager *);
  void executeSetInterface(const InterfaceToken &, ConfigurationManager *);
  void executeDeleteInterface(const InterfaceToken &, ConfigurationManager *);

  void executeShowRoute(const RouteToken &, ConfigurationManager *);
  void executeSetRoute(const RouteToken &, ConfigurationManager *);
  void executeDeleteRoute(const RouteToken &, ConfigurationManager *);

  void executeShowVRF(const VRFToken &, ConfigurationManager *);
  void executeSetVRF(const VRFToken &, ConfigurationManager *);
  void executeDeleteVRF(const VRFToken &, ConfigurationManager *);

  void executeShowArp(const ArpToken &, ConfigurationManager *);
  void executeSetArp(const ArpToken &, ConfigurationManager *);
  void executeDeleteArp(const ArpToken &, ConfigurationManager *);

  void executeShowNdp(const NdpToken &, ConfigurationManager *);
  void executeSetNdp(const NdpToken &, ConfigurationManager *);
  void executeDeleteNdp(const NdpToken &, ConfigurationManager *);

  // Helper: wrap a typed handler into the type-erased Handler signature.
  template <typename TokenT>
  static Handler wrap(void (*fn)(const TokenT &, ConfigurationManager *)) {
    return [fn](const Token &tok, ConfigurationManager *mgr) {
      fn(static_cast<const TokenT &>(tok), mgr);
    };
  }

  CommandDispatcher::CommandDispatcher() { registerDefaults(); }

  void CommandDispatcher::registerDefaults() {
    // Interface handlers
    registerHandler<InterfaceToken>(Verb::Show, wrap(&executeShowInterface));
    registerHandler<InterfaceToken>(Verb::Set, wrap(&executeSetInterface));
    registerHandler<InterfaceToken>(Verb::Delete,
                                    wrap(&executeDeleteInterface));

    // Route handlers
    registerHandler<RouteToken>(Verb::Show, wrap(&executeShowRoute));
    registerHandler<RouteToken>(Verb::Set, wrap(&executeSetRoute));
    registerHandler<RouteToken>(Verb::Delete, wrap(&executeDeleteRoute));

    // VRF handlers
    registerHandler<VRFToken>(Verb::Show, wrap(&executeShowVRF));
    registerHandler<VRFToken>(Verb::Set, wrap(&executeSetVRF));
    registerHandler<VRFToken>(Verb::Delete, wrap(&executeDeleteVRF));

    // ARP handlers
    registerHandler<ArpToken>(Verb::Show, wrap(&executeShowArp));
    registerHandler<ArpToken>(Verb::Set, wrap(&executeSetArp));
    registerHandler<ArpToken>(Verb::Delete, wrap(&executeDeleteArp));

    // NDP handlers
    registerHandler<NdpToken>(Verb::Show, wrap(&executeShowNdp));
    registerHandler<NdpToken>(Verb::Set, wrap(&executeSetNdp));
    registerHandler<NdpToken>(Verb::Delete, wrap(&executeDeleteNdp));
  }

  void CommandDispatcher::dispatch(const std::shared_ptr<Token> &head,
                                   ConfigurationManager *mgr) const {
    if (!head)
      return;

    // Identify the verb from the head token.
    Verb verb;
    if (dynamic_cast<ShowToken *>(head.get()))
      verb = Verb::Show;
    else if (dynamic_cast<SetToken *>(head.get()))
      verb = Verb::Set;
    else if (dynamic_cast<DeleteToken *>(head.get()))
      verb = Verb::Delete;
    else {
      std::cerr << "execute: unknown or unsupported command\n";
      return;
    }

    auto next = head->getNext();
    if (!next) {
      std::cerr << head->toString() << ": missing object\n";
      return;
    }

    // Look up the handler by (verb, target token type).
    const Token &ref = *next;
    Key key{verb, std::type_index(typeid(ref))};
    auto it = handlers_.find(key);
    if (it == handlers_.end()) {
      std::cerr << head->toString() << ": unknown object type\n";
      return;
    }

    it->second(*next, mgr);
  }

} // namespace netcli
