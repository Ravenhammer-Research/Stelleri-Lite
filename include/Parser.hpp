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

#pragma once

#include "Command.hpp"
#include "ConfigurationManager.hpp"
#include "InterfaceToken.hpp"
#include "RouteToken.hpp"
#include "Token.hpp"
#include "VRFToken.hpp"
#include <memory>
#include <string>
#include <vector>

namespace netcli {

  class Parser {
  public:
    Parser() = default;
    ~Parser() = default;

    // Tokenize a raw command line into space-separated tokens.
    std::vector<std::string> tokenize(const std::string &line) const;

    // Parse a token vector into a Command. Returns nullptr on parse error.
    std::unique_ptr<Command>
    parse(const std::vector<std::string> &tokens) const;

    // Main dispatcher: execute a parsed command token chain.
    void executeCommand(const std::shared_ptr<Token> &head,
                        ConfigurationManager *mgr) const;

    // Per-target execute handlers (moved from standalone Execute* files).
    void executeShowInterface(const InterfaceToken &tok,
                              ConfigurationManager *mgr) const;
    void executeSetInterface(const InterfaceToken &tok,
                             ConfigurationManager *mgr) const;
    void executeDeleteInterface(const InterfaceToken &tok,
                                ConfigurationManager *mgr) const;

    void executeShowRoute(const RouteToken &tok,
                          ConfigurationManager *mgr) const;
    void executeSetRoute(const RouteToken &tok,
                         ConfigurationManager *mgr) const;
    void executeDeleteRoute(const RouteToken &tok,
                            ConfigurationManager *mgr) const;

    void executeShowVRF(const VRFToken &tok, ConfigurationManager *mgr) const;
    void executeSetVRF(const VRFToken &tok, ConfigurationManager *mgr) const;
    void executeDeleteVRF(const VRFToken &tok, ConfigurationManager *mgr) const;
  };

} // namespace netcli
