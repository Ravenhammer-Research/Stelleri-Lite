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
    } else if (target == "route") {
      if (idx + 1 < tokens.size()) {
        auto rt = std::make_shared<RouteToken>(tokens[idx + 1]);
        // parse options starting after prefix
        rt->parseOptions(tokens, idx + 2);
        cmd->addToken(rt);
      }
      } else if (target == "route" || target == "routes") {
        if (idx + 1 < tokens.size()) {
          auto rt = std::make_shared<RouteToken>(tokens[idx + 1]);
          // parse options starting after prefix
          rt->parseOptions(tokens, idx + 2);
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
