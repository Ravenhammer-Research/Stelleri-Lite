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

/**
 * @file CommandDispatcher.hpp
 * @brief Type-safe command dispatch replacing dynamic_cast cascades
 *
 * The CommandDispatcher maintains a registry of handlers keyed by
 * (Verb, token type_index). When dispatch() is called, it identifies the
 * verb token, extracts the target token, and looks up the matching handler.
 */

#pragma once

#include "ConfigurationManager.hpp"
#include "Token.hpp"
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <typeindex>

namespace netcli {

/// Verb categories parsed from the command head token.
enum class Verb { Show, Set, Delete };

/// A handler receives the raw target Token* and the ConfigurationManager.
/// Implementations static_cast to the concrete token type (safe because
/// dispatch guarantees the type matches what was registered).
using Handler = std::function<void(const Token &, ConfigurationManager *)>;

class CommandDispatcher {
public:
  CommandDispatcher();

  /// Register a handler for a specific verb + target token type.
  template <typename TokenT>
  void registerHandler(Verb verb, Handler handler) {
    handlers_[{verb, std::type_index(typeid(TokenT))}] = std::move(handler);
  }

  /// Dispatch a parsed command chain. The head token must be a verb token
  /// (ShowToken, SetToken, DeleteToken) whose next() is the target token.
  void dispatch(const std::shared_ptr<Token> &head,
                ConfigurationManager *mgr) const;

private:
  /// Register all built-in handlers.
  void registerDefaults();

  using Key = std::pair<Verb, std::type_index>;
  std::map<Key, Handler> handlers_;
};

} // namespace netcli
