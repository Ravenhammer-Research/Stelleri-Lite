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

#include "ArpConfig.hpp"
#include "CarpInterfaceConfig.hpp"
#include "EpairInterfaceConfig.hpp"
#include "GifInterfaceConfig.hpp"
#include "GreInterfaceConfig.hpp"
#include "IpsecInterfaceConfig.hpp"
#include "LaggInterfaceConfig.hpp"
#include "NdpConfig.hpp"
#include "OvpnInterfaceConfig.hpp"
#include "PflogInterfaceConfig.hpp"
#include "PfsyncInterfaceConfig.hpp"
#include "PolicyConfig.hpp"
#include "SixToFourInterfaceConfig.hpp"
#include "SystemConfigurationManager.hpp"
#include "TapInterfaceConfig.hpp"
#include "TunInterfaceConfig.hpp"
#include "VRFConfig.hpp"
#include "VlanInterfaceConfig.hpp"
#include "VxlanInterfaceConfig.hpp"
#include "WlanInterfaceConfig.hpp"

std::vector<LaggInterfaceConfig> SystemConfigurationManager::GetLaggInterfaces(
    const std::vector<InterfaceConfig> &bases [[maybe_unused]]) const {
  return {};
}

std::vector<GifInterfaceConfig> SystemConfigurationManager::GetGifInterfaces(
    const std::vector<InterfaceConfig> &bases [[maybe_unused]]) const {
  return {};
}

std::vector<OvpnInterfaceConfig> SystemConfigurationManager::GetOvpnInterfaces(
    const std::vector<InterfaceConfig> &bases [[maybe_unused]]) const {
  return {};
}

std::vector<IpsecInterfaceConfig>
SystemConfigurationManager::GetIpsecInterfaces(
    const std::vector<InterfaceConfig> &bases [[maybe_unused]]) const {
  return {};
}

std::vector<GreInterfaceConfig> SystemConfigurationManager::GetGreInterfaces(
    const std::vector<InterfaceConfig> &bases [[maybe_unused]]) const {
  return {};
}

std::vector<VxlanInterfaceConfig>
SystemConfigurationManager::GetVxlanInterfaces(
    const std::vector<InterfaceConfig> &bases [[maybe_unused]]) const {
  return {};
}

std::vector<EpairInterfaceConfig>
SystemConfigurationManager::GetEpairInterfaces(
    const std::vector<InterfaceConfig> &bases [[maybe_unused]]) const {
  return {};
}

std::vector<WlanInterfaceConfig> SystemConfigurationManager::GetWlanInterfaces(
    const std::vector<InterfaceConfig> &bases [[maybe_unused]]) const {
  return {};
}

std::vector<CarpInterfaceConfig> SystemConfigurationManager::GetCarpInterfaces(
    const std::vector<InterfaceConfig> &bases [[maybe_unused]]) const {
  return {};
}

std::vector<NdpConfig> SystemConfigurationManager::GetNdpEntries(
    const std::optional<std::string> &ip_filter [[maybe_unused]],
    const std::optional<std::string> &iface_filter [[maybe_unused]]) const {
  return {};
}

bool SystemConfigurationManager::SetNdpEntry(
    const std::string &ip [[maybe_unused]],
    const std::string &mac [[maybe_unused]],
    const std::optional<std::string> &iface [[maybe_unused]],
    bool temp [[maybe_unused]]) const {
  return false;
}

bool SystemConfigurationManager::DeleteNdpEntry(
    const std::string &ip [[maybe_unused]],
    const std::optional<std::string> &iface [[maybe_unused]]) const {
  return false;
}

void SystemConfigurationManager::CreateLagg(const std::string &name
                                            [[maybe_unused]]) const {}
void SystemConfigurationManager::SaveLagg(const LaggInterfaceConfig &lac
                                          [[maybe_unused]]) const {}
void SystemConfigurationManager::CreateGif(const std::string &name
                                           [[maybe_unused]]) const {}
void SystemConfigurationManager::SaveGif(const GifInterfaceConfig &gif
                                         [[maybe_unused]]) const {}
void SystemConfigurationManager::CreateOvpn(const std::string &name
                                            [[maybe_unused]]) const {}
void SystemConfigurationManager::SaveOvpn(const OvpnInterfaceConfig &ovpn
                                          [[maybe_unused]]) const {}
void SystemConfigurationManager::CreateIpsec(const std::string &name
                                             [[maybe_unused]]) const {}
void SystemConfigurationManager::SaveIpsec(const IpsecInterfaceConfig &ipsec
                                           [[maybe_unused]]) const {}
void SystemConfigurationManager::CreateWlan(const std::string &name
                                            [[maybe_unused]]) const {}
void SystemConfigurationManager::SaveWlan(const WlanInterfaceConfig &wlan
                                          [[maybe_unused]]) const {}
void SystemConfigurationManager::CreateGre(const std::string &name
                                           [[maybe_unused]]) const {}
void SystemConfigurationManager::SaveGre(const GreInterfaceConfig &gre
                                         [[maybe_unused]]) const {}
void SystemConfigurationManager::CreateVxlan(const std::string &name
                                             [[maybe_unused]]) const {}
void SystemConfigurationManager::SaveVxlan(const VxlanInterfaceConfig &vxlan
                                           [[maybe_unused]]) const {}
void SystemConfigurationManager::SaveCarp(const CarpInterfaceConfig &carp
                                          [[maybe_unused]]) const {}
void SystemConfigurationManager::CreateEpair(const std::string &name
                                             [[maybe_unused]]) const {}
void SystemConfigurationManager::SaveEpair(const EpairInterfaceConfig &epair
                                           [[maybe_unused]]) const {}

std::vector<PolicyConfig> SystemConfigurationManager::GetPolicies(
    const std::optional<uint32_t> &acl_filter [[maybe_unused]]) const {
  return {};
}
void SystemConfigurationManager::SetPolicy(const PolicyConfig &pc
                                           [[maybe_unused]]) const {}
void SystemConfigurationManager::DeletePolicy(const PolicyConfig &pc
                                              [[maybe_unused]]) const {}

void SystemConfigurationManager::CreateSixToFour(const std::string &name
                                                 [[maybe_unused]]) const {}
void SystemConfigurationManager::SaveSixToFour(const SixToFourInterfaceConfig &t
                                               [[maybe_unused]]) const {}
void SystemConfigurationManager::DestroySixToFour(const std::string &name
                                                  [[maybe_unused]]) const {}
std::vector<SixToFourInterfaceConfig>
SystemConfigurationManager::GetSixToFourInterfaces(
    const std::vector<InterfaceConfig> &bases [[maybe_unused]]) const {
  return {};
}

void SystemConfigurationManager::CreatePflog(const std::string &name
                                             [[maybe_unused]]) const {}
void SystemConfigurationManager::SavePflog(const PflogInterfaceConfig &p
                                           [[maybe_unused]]) const {}
void SystemConfigurationManager::DestroyPflog(const std::string &name
                                              [[maybe_unused]]) const {}
std::vector<PflogInterfaceConfig>
SystemConfigurationManager::GetPflogInterfaces(
    const std::vector<InterfaceConfig> &bases [[maybe_unused]]) const {
  return {};
}

void SystemConfigurationManager::CreatePfsync(const std::string &name
                                              [[maybe_unused]]) const {}
void SystemConfigurationManager::SavePfsync(const PfsyncInterfaceConfig &p
                                            [[maybe_unused]]) const {}
void SystemConfigurationManager::DestroyPfsync(const std::string &name
                                               [[maybe_unused]]) const {}
std::vector<PfsyncInterfaceConfig>
SystemConfigurationManager::GetPfsyncInterfaces(
    const std::vector<InterfaceConfig> &bases [[maybe_unused]]) const {
  return {};
}
