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

#include "LoopBackTableFormatter.hpp"
#include "InterfaceConfig.hpp"
#include "InterfaceFlags.hpp"
#include <iomanip>
#include <sstream>

std::string
LoopBackTableFormatter::format(const std::vector<InterfaceConfig> &items) {
  addColumn("Interface", "Interface", 10, 4, true);
  addColumn("Address", "Address", 5, 7, true);
  addColumn("Status", "Status", 6, 6, true);

  for (const auto &ic : items) {
    if (ic.type != InterfaceType::Loopback)
      continue;

    std::vector<std::string> addrs;
    if (ic.address)
      addrs.push_back(ic.address->toString());
    for (const auto &a : ic.aliases) {
      if (a)
        addrs.push_back(a->toString());
    }

    std::ostringstream aoss;
    for (size_t i = 0; i < addrs.size(); ++i) {
      if (i)
        aoss << '\n';
      aoss << addrs[i];
    }
    std::string addrCell = addrs.empty() ? std::string("-") : aoss.str();

    std::string status = "-";
    if (ic.flags) {
      if (hasFlag(*ic.flags, InterfaceFlag::RUNNING))
        status = "active";
      else if (hasFlag(*ic.flags, InterfaceFlag::UP))
        status = "no-carrier";
      else
        status = "down";
    }

    addRow({ic.name, addrCell, status});
  }

  return renderTable(80);
}
