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

#include "ConfigurationGenerator.hpp"
#include "GenerateArpCommands.hpp"
#include "GenerateCarpCommands.hpp"
#include "GenerateGifCommands.hpp"
#include "GenerateGreCommands.hpp"
#include "GenerateIpsecCommands.hpp"
#include "GenerateNdpCommands.hpp"
#include "GenerateOvpnCommands.hpp"
#include "GeneratePflogCommands.hpp"
#include "GeneratePfsyncCommands.hpp"
#include "GenerateSixToFourCommands.hpp"
#include "GenerateTapCommands.hpp"
#include "GenerateTunCommands.hpp"
#include "GenerateVxlanCommands.hpp"
#include "GenerateWireGuardCommands.hpp"
#include "GenerateWlanCommands.hpp"

namespace netcli {

  void
  ConfigurationGenerator::generateConfiguration(ConfigurationManager &mgr) {
    std::set<std::string> processedInterfaces;

    // Generate VRFs
    generateVRFs(mgr);

    // Generate interfaces with addresses
    generateLoopbacks(mgr, processedInterfaces);
    generateEpairs(mgr, processedInterfaces);
    generateBasicInterfaces(mgr, processedInterfaces);
    generateBridges(mgr, processedInterfaces);
    generateLaggs(mgr, processedInterfaces);
    generateVLANs(mgr, processedInterfaces);
    // Generate tunnels by specific type
    generateTunCommands(mgr, processedInterfaces);
    generateGifCommands(mgr, processedInterfaces);
    generateOvpnCommands(mgr, processedInterfaces);
    generateIpsecCommands(mgr, processedInterfaces);
    generateGreCommands(mgr, processedInterfaces);
    generateVxlanCommands(mgr, processedInterfaces);
    generateWlanCommands(mgr, processedInterfaces);
    generateCarpCommands(mgr, processedInterfaces);
    generateTapCommands(mgr, processedInterfaces);
    generatePflogCommands(mgr, processedInterfaces);
    generatePfsyncCommands(mgr, processedInterfaces);
    generateWireGuardCommands(mgr, processedInterfaces);
    generateSixToFourCommands(mgr, processedInterfaces);

    // Generate routes
    generateRoutes(mgr);

    // Generate ARP/NDP static entries
    generateArpCommands(mgr);
    generateNdpCommands(mgr);
  }

} // namespace netcli
