/*
 * Copyright (c) 2026, Ravenhammer Research Inc.
 * All rights reserved.
 */

#include "ConfigurationManager.hpp"
#include "InterfaceToken.hpp"
#include "TapInterfaceConfig.hpp"
#include "TapTableFormatter.hpp"
#include <iostream>

class TapInterfaceToken : public InterfaceToken {
public:
  using InterfaceToken::InterfaceToken;
};

std::string InterfaceToken::toString(TapInterfaceConfig *cfg) {
  if (!cfg)
    return std::string();
  return InterfaceToken::toString(static_cast<InterfaceConfig *>(cfg));
}

bool InterfaceToken::parseTapKeywords(std::shared_ptr<InterfaceToken> &tok,
                                      const std::vector<std::string> &tokens,
                                      size_t &cur) {
  (void)tok;
  (void)tokens;
  (void)cur;
  return false;
}

std::vector<std::string>
InterfaceToken::tapCompletions(const std::string &prev) {
  (void)prev;
  return {};
}

void InterfaceToken::setTapInterface(const InterfaceToken &tok,
                                     ConfigurationManager *mgr,
                                     InterfaceConfig &base, bool exists) {
  TapInterfaceConfig tc(base);
  tc.save(*mgr);
  std::cout << "set interface: " << (exists ? "updated" : "created") << " tap '"
            << tok.name() << "'\n";
}

std::string
InterfaceToken::showTapInterfaces(const std::vector<InterfaceConfig> &ifaces,
                                  ConfigurationManager *) {
  TapTableFormatter f;
  return f.format(ifaces);
}
