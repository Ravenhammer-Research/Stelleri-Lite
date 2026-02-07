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

#include "ConfigurationManager.hpp"
#include "DeleteToken.hpp"
#include "InterfaceToken.hpp"
#include "Parser.hpp"
#include "RouteToken.hpp"
#include "SetToken.hpp"
#include "ShowToken.hpp"
#include "VRFToken.hpp"
#include <iostream>

namespace netcli {

  void Parser::executeCommand(const std::shared_ptr<Token> &head,
                              ConfigurationManager *mgr) const {
    if (!head)
      return;

    if (dynamic_cast<ShowToken *>(head.get())) {
      auto next = head->getNext();
      if (!next)
        return;
      if (auto iftok = dynamic_cast<InterfaceToken *>(next.get())) {
        executeShowInterface(*iftok, mgr);
        return;
      }
      if (auto rt = dynamic_cast<RouteToken *>(next.get())) {
        executeShowRoute(*rt, mgr);
        return;
      }
      if (auto vt = dynamic_cast<VRFToken *>(next.get())) {
        executeShowVRF(*vt, mgr);
        return;
      }
    }

    if (dynamic_cast<SetToken *>(head.get())) {
      auto next = head->getNext();
      if (!next)
        return;
      if (auto iftok = dynamic_cast<InterfaceToken *>(next.get())) {
        executeSetInterface(*iftok, mgr);
        return;
      }
      if (auto rt = dynamic_cast<RouteToken *>(next.get())) {
        executeSetRoute(*rt, mgr);
        return;
      }
      if (auto vt = dynamic_cast<VRFToken *>(next.get())) {
        executeSetVRF(*vt, mgr);
        return;
      }
    }

    if (dynamic_cast<DeleteToken *>(head.get())) {
      auto next = head->getNext();
      if (!next)
        return;
      if (auto iftok = dynamic_cast<InterfaceToken *>(next.get())) {
        executeDeleteInterface(*iftok, mgr);
        return;
      }
      if (auto rt = dynamic_cast<RouteToken *>(next.get())) {
        executeDeleteRoute(*rt, mgr);
        return;
      }
      if (auto vt = dynamic_cast<VRFToken *>(next.get())) {
        executeDeleteVRF(*vt, mgr);
        return;
      }
    }

    std::cout << "execute: unknown or unsupported command\n";
  }

} // namespace netcli
