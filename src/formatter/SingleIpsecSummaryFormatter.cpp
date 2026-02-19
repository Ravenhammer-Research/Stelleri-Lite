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

#include "SingleIpsecSummaryFormatter.hpp"
#include "SingleInterfaceSummaryFormatter.hpp"
#include <iomanip>
#include <sstream>

std::string
SingleIpsecSummaryFormatter::format(const IpsecInterfaceConfig &ipsec) const {
  SingleInterfaceSummaryFormatter base;
  std::string out = base.format(ipsec);

  std::ostringstream oss;
  if (ipsec.tunnel_vrf)
    oss << "Tunnel VRF: " << *ipsec.tunnel_vrf << "\n";
  if (ipsec.source)
    oss << "Tunnel Src: " << ipsec.source->toString() << "\n";
  if (ipsec.destination)
    oss << "Tunnel Dst: " << ipsec.destination->toString() << "\n";
  if (ipsec.reqid)
    oss << "Reqid: " << *ipsec.reqid << "\n";

  for (const auto &sa : ipsec.security_associations) {
    oss << "SA: protocol " << sa.protocol << " spi 0x" << std::hex << sa.spi
        << std::dec << " " << sa.src << " -> " << sa.dst << " auth "
        << sa.algorithm;
    if (sa.enc_algorithm)
      oss << " enc " << *sa.enc_algorithm;
    oss << "\n";
  }

  for (const auto &sp : ipsec.security_policies) {
    oss << "SP: direction " << sp.direction << " policy \"" << sp.policy
        << "\"";
    if (sp.reqid)
      oss << " reqid " << *sp.reqid;
    oss << "\n";
  }

  out += oss.str();
  return out;
}
