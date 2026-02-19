/*
 * Copyright (c) 2026, Ravenhammer Research Inc.
 * All rights reserved.
 */

#include "ConfigurationManager.hpp"
#include "InterfaceToken.hpp"
#include "LoopBackInterfaceConfig.hpp"
#include "LoopBackTableFormatter.hpp"
#include <iostream>

class LoopbackInterfaceToken : public InterfaceToken {
public:
  using InterfaceToken::InterfaceToken;
};

bool InterfaceToken::parseLoopbackKeywords(
    std::shared_ptr<InterfaceToken> &tok,
    const std::vector<std::string> &tokens, size_t &cur) {
  (void)tok;
  (void)tokens;
  (void)cur;
  return false;
}

std::vector<std::string>
InterfaceToken::loopbackCompletions(const std::string &prev) {
  (void)prev;
  return {};
}

void InterfaceToken::setLoopbackInterface(const InterfaceToken &tok,
                                          ConfigurationManager *mgr,
                                          InterfaceConfig &base, bool exists) {
  LoopBackInterfaceConfig lbc(base);
  lbc.save(*mgr);
  std::cout << "set interface: " << (exists ? "updated" : "created")
            << " loopback '" << tok.name() << "'\n";
}

std::string InterfaceToken::showLoopbackInterfaces(
    const std::vector<InterfaceConfig> &ifaces, ConfigurationManager *) {
  LoopBackTableFormatter f;
  return f.format(ifaces);
}
