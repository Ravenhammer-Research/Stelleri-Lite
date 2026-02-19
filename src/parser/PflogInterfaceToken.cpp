/*
 * Copyright (c) 2026, Ravenhammer Research Inc.
 * All rights reserved.
 */

#include "ConfigurationManager.hpp"
#include "InterfaceToken.hpp"
#include "PflogInterfaceConfig.hpp"
#include "PflogTableFormatter.hpp"
#include <iostream>

class PflogInterfaceToken : public InterfaceToken {
public:
  using InterfaceToken::InterfaceToken;
};

bool InterfaceToken::parsePflogKeywords(std::shared_ptr<InterfaceToken> &tok,
                                        const std::vector<std::string> &tokens,
                                        size_t &cur) {
  (void)tok;
  (void)tokens;
  (void)cur;
  return false;
}

std::vector<std::string>
InterfaceToken::pflogCompletions(const std::string &prev) {
  (void)prev;
  return {};
}

void InterfaceToken::setPflogInterface(const InterfaceToken &tok,
                                       ConfigurationManager *mgr,
                                       InterfaceConfig &base, bool exists) {
  PflogInterfaceConfig pc(base);
  pc.save(*mgr);
  std::cout << "set interface: " << (exists ? "updated" : "created")
            << " pflog '" << tok.name() << "'\n";
}

std::string
InterfaceToken::showPflogInterfaces(const std::vector<InterfaceConfig> &ifaces,
                                    ConfigurationManager *) {
  PflogTableFormatter f;
  return f.format(ifaces);
}
