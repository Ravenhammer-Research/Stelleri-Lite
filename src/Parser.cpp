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

#include "Parser.hpp"
#include "Command.hpp"
#include "DeleteToken.hpp"
#include "InterfaceToken.hpp"
#include "RouteToken.hpp"
#include "SetToken.hpp"
#include "ShowToken.hpp"
#include "VRFToken.hpp"
#include <iostream>
#include <sstream>

namespace netcli {
  std::vector<std::string> Parser::tokenize(const std::string &line) const {
    std::vector<std::string> out;
    std::istringstream iss(line);
    std::string tok;
    while (iss >> tok) {
      out.push_back(tok);
    }
    return out;
  }

  std::unique_ptr<Command>
  Parser::parse(const std::vector<std::string> &tokens) const {
    if (tokens.empty())
      return nullptr;

    auto cmd = std::make_unique<Command>();
    size_t idx = 0;
    const std::string &verb = tokens[idx];
    if (verb == "show") {
      cmd->addToken(std::make_shared<ShowToken>());
      ++idx;
    } else if (verb == "set") {
      cmd->addToken(std::make_shared<SetToken>());
      ++idx;
    } else if (verb == "delete") {
      cmd->addToken(std::make_shared<DeleteToken>());
      ++idx;
    } else {
      return nullptr;
    }

    if (idx >= tokens.size())
      return cmd;

    const std::string &target = tokens[idx];
    if (target == "interfaces" || target == "interface") {
      size_t next = 0;
      auto itok = InterfaceToken::parseFromTokens(tokens, idx, next);
      if (itok)
        cmd->addToken(itok);
    } else if (target == "route" || target == "routes") {
      if (idx + 1 < tokens.size()) {
        // Pass the next token as the potential prefix to the RouteToken and
        // let the token decide whether it's an actual prefix or an option.
        auto rt = std::make_shared<RouteToken>(tokens[idx + 1]);
        rt->parseOptions(tokens, idx + 1);
        cmd->addToken(rt);
      } else {
        // support `show routes` with no prefix
        auto rt = std::make_shared<RouteToken>(std::string());
        cmd->addToken(rt);
      }
    } else if (target == "vrf") {
      if (idx + 1 < tokens.size()) {
        auto vt = std::make_shared<VRFToken>(tokens[idx + 1]);
        cmd->addToken(vt);
      }
    }

    return cmd;
  }

  // `executeShowInterface` is implemented in src/ExecuteShowInterface.cpp

} // namespace netcli
