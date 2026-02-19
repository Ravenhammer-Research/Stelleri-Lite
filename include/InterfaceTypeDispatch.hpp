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
 * @file InterfaceTypeDispatch.hpp
 * @brief Per-InterfaceType function-pointer bundle used by InterfaceToken.
 *
 * One static table (built inside InterfaceToken::dispatch()) maps every
 * concrete InterfaceType to its CLI type name, default FreeBSD group, and
 * the five per-type handler functions (completions, parseKeywords, set,
 * showSingle, showTable).  This eliminates the repetitive 19-case switch
 * statements that previously appeared throughout InterfaceToken.cpp.
 */

#pragma once

#include <memory>
#include <string>
#include <vector>

class InterfaceToken;
class InterfaceConfig;
class ConfigurationManager;

/// Function-pointer bundle for per-type interface dispatch.
/// Looked up via InterfaceToken::dispatch(InterfaceType).
struct InterfaceTypeDispatch {
  const char *typeName;     ///< CLI type keyword ("bridge", "vlan", …)
  const char *defaultGroup; ///< FreeBSD auto-assigned group (nullptr if none)

  /// Tab-completion provider for type-specific keywords.
  using CompletionsFn = std::vector<std::string> (*)(const std::string &);

  /// Parser for type-specific keyword arguments.
  using ParseKeywordsFn = bool (*)(std::shared_ptr<InterfaceToken> &,
                                   const std::vector<std::string> &, size_t &);

  /// Execute 'set interface … type X' — builds type config and saves.
  using SetFn = void (*)(const InterfaceToken &, ConfigurationManager *,
                         InterfaceConfig &, bool);

  /// Single-interface summary display.  Returns true if handled.
  using ShowSingleFn = bool (*)(const InterfaceConfig &,
                                ConfigurationManager *);

  /// Multi-interface table formatter.  Returns the rendered table string.
  using ShowTableFn = std::string (*)(const std::vector<InterfaceConfig> &,
                                      ConfigurationManager *);

  CompletionsFn completions;     ///< may be nullptr
  ParseKeywordsFn parseKeywords; ///< may be nullptr
  SetFn setInterface;            ///< may be nullptr
  ShowSingleFn showInterface;    ///< nullptr → fall back to generic formatter
  ShowTableFn
      showInterfaces; ///< nullptr → fall back to InterfaceTableFormatter
};
