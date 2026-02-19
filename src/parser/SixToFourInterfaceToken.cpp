/*
 * Copyright (c) 2026, Ravenhammer Research Inc.
 * All rights reserved.
 */

#include "ConfigurationManager.hpp"
#include "InterfaceToken.hpp"
#include "SixToFourInterfaceConfig.hpp"
#include "SixToFourTableFormatter.hpp"
#include <iostream>

class SixToFourInterfaceToken : public InterfaceToken {
public:
  using InterfaceToken::InterfaceToken;
};

std::string InterfaceToken::toString(SixToFourInterfaceConfig *cfg) {
  if (!cfg)
    return std::string();
  return InterfaceToken::toString(static_cast<InterfaceConfig *>(cfg));
}

bool InterfaceToken::parseSixToFourKeywords(
    std::shared_ptr<InterfaceToken> &tok,
    const std::vector<std::string> &tokens, size_t &cur) {
  (void)tok;
  (void)tokens;
  (void)cur;
  return false;
}

std::vector<std::string>
InterfaceToken::sixToFourCompletions(const std::string &prev) {
  (void)prev;
  return {};
}

void InterfaceToken::setSixToFourInterface(const InterfaceToken &tok,
                                           ConfigurationManager *mgr,
                                           InterfaceConfig &base, bool exists) {
  SixToFourInterfaceConfig sc(base);
  sc.save(*mgr);
  std::cout << "set interface: " << (exists ? "updated" : "created") << " stf '"
            << tok.name() << "'\n";
}

std::string InterfaceToken::showSixToFourInterfaces(
    const std::vector<InterfaceConfig> &ifaces, ConfigurationManager *) {
  SixToFourTableFormatter f;
  return f.format(ifaces);
}
