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

/**
 * @file InterfaceToken.hpp
 * @brief Parser token for interface set/show/delete commands
 */

#pragma once

#include "InterfaceType.hpp"
#include "InterfaceTypeDispatch.hpp"
#include <iostream>
#include <optional>
#include <string>

#include "BridgeInterfaceConfig.hpp"
#include "BridgeMemberConfig.hpp"
#include "CarpInterfaceConfig.hpp"
#include "EpairInterfaceConfig.hpp"
#include "GifInterfaceConfig.hpp"
#include "GreInterfaceConfig.hpp"
#include "InterfaceConfig.hpp"
#include "IpsecInterfaceConfig.hpp"
#include "LaggInterfaceConfig.hpp"
#include "LoopBackInterfaceConfig.hpp"
#include "OvpnInterfaceConfig.hpp"
#include "SixToFourInterfaceConfig.hpp"
#include "TapInterfaceConfig.hpp"
#include "Token.hpp"
#include "TunInterfaceConfig.hpp"
#include "VRFConfig.hpp"
#include "VlanInterfaceConfig.hpp"
#include "VxlanInterfaceConfig.hpp"
#include "WireGuardInterfaceConfig.hpp"
#include "WlanInterfaceConfig.hpp"

class InterfaceToken : public Token {
public:
  InterfaceToken(InterfaceType t, std::string name);

  /**
   * @brief Render an InterfaceConfig to a command string
   */
  static std::string toString(InterfaceConfig *cfg);

  /// Central type→handler dispatch table lookup.
  /// Returns nullptr for Unknown or unmapped types.
  static const InterfaceTypeDispatch *dispatch(InterfaceType t);

  /* overloads for specific interface-related configs */
  static std::string toString(BridgeInterfaceConfig *cfg);
  static std::string toString(CarpInterfaceConfig *cfg);
  static std::string toString(GreInterfaceConfig *cfg);
  static std::string toString(LaggInterfaceConfig *cfg);
  static std::string toString(SixToFourInterfaceConfig *cfg);
  static std::string toString(TapInterfaceConfig *cfg);

  static std::string toString(TunInterfaceConfig *cfg);
  static std::string toString(GifInterfaceConfig *cfg);
  static std::string toString(OvpnInterfaceConfig *cfg);
  static std::string toString(IpsecInterfaceConfig *cfg);
  static std::string toString(VlanInterfaceConfig *cfg);
  static std::string toString(VxlanInterfaceConfig *cfg);
  static std::string toString(WlanInterfaceConfig *cfg);
  static std::string toString(WireGuardInterfaceConfig *cfg);
  std::vector<std::string>
  autoComplete(std::string_view partial) const override;
  std::vector<std::string>
  autoComplete(const std::vector<std::string> &tokens, std::string_view partial,
               ConfigurationManager *mgr) const override;
  std::unique_ptr<Token> clone() const override;

  /// Execute a 'set interface' command using this token's parsed state.
  void executeSet(ConfigurationManager *mgr) const;
  /// Execute a 'show interface' command using this token's parsed state.
  void executeShow(ConfigurationManager *mgr) const;
  /// Execute a 'delete interface' command using this token's parsed state.
  void executeDelete(ConfigurationManager *mgr) const;

  const std::string &name() const { return name_; }
  InterfaceType type() const { return type_; }
  std::optional<int> vrf;
  std::optional<std::string> group;
  std::optional<int> tunnel_vrf;
  std::optional<std::string> address;
  std::optional<int> address_family; // AF_INET or AF_INET6 when inet/inet6 used
  std::optional<int> mtu;
  std::optional<bool> status;             // true=up, false=down
  std::optional<std::string> description; ///< Interface description text

  // Tunnel source/destination (tun, gif, ovpn, ipsec, gre)
  std::optional<std::string> source;
  std::optional<std::string> destination;

  // --- IPsec SA sub-command fields ---
  std::optional<IpsecSA> ipsec_sa;

  // --- IPsec SP sub-command fields ---
  std::optional<IpsecSP> ipsec_sp;

  // --- IPsec interface reqid ---
  std::optional<uint32_t> ipsec_reqid;

  // Sub-config blocks — type-specific data lives in the config objects
  std::optional<BridgeInterfaceConfig> bridge;
  std::optional<LaggInterfaceConfig> lagg;
  std::optional<VlanInterfaceConfig> vlan;
  std::optional<TunInterfaceConfig> tun;
  std::optional<GifInterfaceConfig> gif;
  std::optional<OvpnInterfaceConfig> ovpn;
  std::optional<IpsecInterfaceConfig> ipsec;
  std::optional<VxlanInterfaceConfig> vxlan;
  std::optional<WlanInterfaceConfig> wlan;
  std::optional<GreInterfaceConfig> gre;
  std::optional<CarpInterfaceConfig> carp;

  // WireGuard
  std::optional<uint16_t> wg_listen_port;

  // (Rendering moved to execute handlers. Token is parse-only.)

  // Parse an `interfaces` sequence starting at index `start` in `tokens`.
  // `start` should point to the "interfaces" token. On success, returns a
  // shared_ptr to an InterfaceToken and sets `next` to the index after consumed
  // tokens. Recognized forms:
  //  - interfaces
  //  - interfaces <type> <name>   where <type> is ethernet|loopback|pppoe
  static std::shared_ptr<InterfaceToken>
  parseFromTokens(const std::vector<std::string> &tokens, size_t start,
                  size_t &next);

protected:
  /// Parse keyword arguments from tokens starting at cur, advancing cur past
  /// consumed tokens. Dispatches to per-type keyword parsers based on type().
  static void parseKeywords(std::shared_ptr<InterfaceToken> &tok,
                            const std::vector<std::string> &tokens,
                            size_t &cur);

  // Per-type keyword parsers — each handles type-specific keywords and returns
  // true if it consumed any tokens at cur, false otherwise.
  static bool parseBridgeKeywords(std::shared_ptr<InterfaceToken> &tok,
                                  const std::vector<std::string> &tokens,
                                  size_t &cur);
  static bool parseVlanKeywords(std::shared_ptr<InterfaceToken> &tok,
                                const std::vector<std::string> &tokens,
                                size_t &cur);
  static bool parseLaggKeywords(std::shared_ptr<InterfaceToken> &tok,
                                const std::vector<std::string> &tokens,
                                size_t &cur);
  static bool parseTunKeywords(std::shared_ptr<InterfaceToken> &tok,
                               const std::vector<std::string> &tokens,
                               size_t &cur);
  static bool parseGifKeywords(std::shared_ptr<InterfaceToken> &tok,
                               const std::vector<std::string> &tokens,
                               size_t &cur);
  static bool parseOvpnKeywords(std::shared_ptr<InterfaceToken> &tok,
                                const std::vector<std::string> &tokens,
                                size_t &cur);
  static bool parseIpsecKeywords(std::shared_ptr<InterfaceToken> &tok,
                                 const std::vector<std::string> &tokens,
                                 size_t &cur);
  static bool parseGreKeywords(std::shared_ptr<InterfaceToken> &tok,
                               const std::vector<std::string> &tokens,
                               size_t &cur);
  static bool parseCarpKeywords(std::shared_ptr<InterfaceToken> &tok,
                                const std::vector<std::string> &tokens,
                                size_t &cur);
  static bool parseVxlanKeywords(std::shared_ptr<InterfaceToken> &tok,
                                 const std::vector<std::string> &tokens,
                                 size_t &cur);
  static bool parseWlanKeywords(std::shared_ptr<InterfaceToken> &tok,
                                const std::vector<std::string> &tokens,
                                size_t &cur);
  static bool parseWireGuardKeywords(std::shared_ptr<InterfaceToken> &tok,
                                     const std::vector<std::string> &tokens,
                                     size_t &cur);
  static bool parseTapKeywords(std::shared_ptr<InterfaceToken> &tok,
                               const std::vector<std::string> &tokens,
                               size_t &cur);
  static bool parseSixToFourKeywords(std::shared_ptr<InterfaceToken> &tok,
                                     const std::vector<std::string> &tokens,
                                     size_t &cur);
  static bool parsePflogKeywords(std::shared_ptr<InterfaceToken> &tok,
                                 const std::vector<std::string> &tokens,
                                 size_t &cur);
  static bool parsePfsyncKeywords(std::shared_ptr<InterfaceToken> &tok,
                                  const std::vector<std::string> &tokens,
                                  size_t &cur);
  static bool parseEpairKeywords(std::shared_ptr<InterfaceToken> &tok,
                                 const std::vector<std::string> &tokens,
                                 size_t &cur);
  static bool parseLoopbackKeywords(std::shared_ptr<InterfaceToken> &tok,
                                    const std::vector<std::string> &tokens,
                                    size_t &cur);

  // Per-type autocompletion providers — return type-specific keywords when
  // prev is empty, or value suggestions for a type-specific keyword.
  static std::vector<std::string> typeCompletions(InterfaceType t,
                                                  const std::string &prev);
  static std::vector<std::string> bridgeCompletions(const std::string &prev);
  static std::vector<std::string> vlanCompletions(const std::string &prev);
  static std::vector<std::string> laggCompletions(const std::string &prev);
  static std::vector<std::string> tunCompletions(const std::string &prev);
  static std::vector<std::string> gifCompletions(const std::string &prev);
  static std::vector<std::string> ovpnCompletions(const std::string &prev);
  static std::vector<std::string> ipsecCompletions(const std::string &prev);
  static std::vector<std::string> greCompletions(const std::string &prev);
  static std::vector<std::string> carpCompletions(const std::string &prev);
  static std::vector<std::string> vxlanCompletions(const std::string &prev);
  static std::vector<std::string> wlanCompletions(const std::string &prev);
  static std::vector<std::string> wireGuardCompletions(const std::string &prev);
  static std::vector<std::string> tapCompletions(const std::string &prev);
  static std::vector<std::string> sixToFourCompletions(const std::string &prev);
  static std::vector<std::string> pflogCompletions(const std::string &prev);
  static std::vector<std::string> pfsyncCompletions(const std::string &prev);
  static std::vector<std::string> epairCompletions(const std::string &prev);
  static std::vector<std::string> loopbackCompletions(const std::string &prev);

  // Per-type set execution — builds the type-specific config, saves it, prints.
  static void setBridgeInterface(const InterfaceToken &tok,
                                 ConfigurationManager *mgr,
                                 InterfaceConfig &base, bool exists);
  static void setVlanInterface(const InterfaceToken &tok,
                               ConfigurationManager *mgr, InterfaceConfig &base,
                               bool exists);
  static void setLaggInterface(const InterfaceToken &tok,
                               ConfigurationManager *mgr, InterfaceConfig &base,
                               bool exists);
  static void setTunInterface(const InterfaceToken &tok,
                              ConfigurationManager *mgr, InterfaceConfig &base,
                              bool exists);
  static void setGifInterface(const InterfaceToken &tok,
                              ConfigurationManager *mgr, InterfaceConfig &base,
                              bool exists);
  static void setOvpnInterface(const InterfaceToken &tok,
                               ConfigurationManager *mgr, InterfaceConfig &base,
                               bool exists);
  static void setIpsecInterface(const InterfaceToken &tok,
                                ConfigurationManager *mgr,
                                InterfaceConfig &base, bool exists);
  static void setGreInterface(const InterfaceToken &tok,
                              ConfigurationManager *mgr, InterfaceConfig &base,
                              bool exists);
  static void setCarpInterface(const InterfaceToken &tok,
                               ConfigurationManager *mgr, InterfaceConfig &base,
                               bool exists);
  static void setVxlanInterface(const InterfaceToken &tok,
                                ConfigurationManager *mgr,
                                InterfaceConfig &base, bool exists);
  static void setWlanInterface(const InterfaceToken &tok,
                               ConfigurationManager *mgr, InterfaceConfig &base,
                               bool exists);
  static void setWireGuardInterface(const InterfaceToken &tok,
                                    ConfigurationManager *mgr,
                                    InterfaceConfig &base, bool exists);
  static void setTapInterface(const InterfaceToken &tok,
                              ConfigurationManager *mgr, InterfaceConfig &base,
                              bool exists);
  static void setSixToFourInterface(const InterfaceToken &tok,
                                    ConfigurationManager *mgr,
                                    InterfaceConfig &base, bool exists);
  static void setPflogInterface(const InterfaceToken &tok,
                                ConfigurationManager *mgr,
                                InterfaceConfig &base, bool exists);
  static void setPfsyncInterface(const InterfaceToken &tok,
                                 ConfigurationManager *mgr,
                                 InterfaceConfig &base, bool exists);
  static void setEpairInterface(const InterfaceToken &tok,
                                ConfigurationManager *mgr,
                                InterfaceConfig &base, bool exists);
  static void setLoopbackInterface(const InterfaceToken &tok,
                                   ConfigurationManager *mgr,
                                   InterfaceConfig &base, bool exists);

  // Per-type single-interface show — returns true if it handled the display.
  static bool showBridgeInterface(const InterfaceConfig &ic,
                                  ConfigurationManager *mgr);
  static bool showVlanInterface(const InterfaceConfig &ic,
                                ConfigurationManager *mgr);
  static bool showLaggInterface(const InterfaceConfig &ic,
                                ConfigurationManager *mgr);
  static bool showTunInterface(const InterfaceConfig &ic,
                               ConfigurationManager *mgr);
  static bool showGifInterface(const InterfaceConfig &ic,
                               ConfigurationManager *mgr);
  static bool showOvpnInterface(const InterfaceConfig &ic,
                                ConfigurationManager *mgr);
  static bool showIpsecInterface(const InterfaceConfig &ic,
                                 ConfigurationManager *mgr);
  static bool showGreInterface(const InterfaceConfig &ic,
                               ConfigurationManager *mgr);
  static bool showCarpInterface(const InterfaceConfig &ic,
                                ConfigurationManager *mgr);
  static bool showVxlanInterface(const InterfaceConfig &ic,
                                 ConfigurationManager *mgr);
  static bool showWlanInterface(const InterfaceConfig &ic,
                                ConfigurationManager *mgr);
  static bool showEpairInterface(const InterfaceConfig &ic,
                                 ConfigurationManager *mgr);

  // Per-type multi-interface table show — returns formatted table string.
  static std::string
  showBridgeInterfaces(const std::vector<InterfaceConfig> &ifaces,
                       ConfigurationManager *mgr);
  static std::string
  showVlanInterfaces(const std::vector<InterfaceConfig> &ifaces,
                     ConfigurationManager *mgr);
  static std::string
  showLaggInterfaces(const std::vector<InterfaceConfig> &ifaces,
                     ConfigurationManager *mgr);
  static std::string
  showTunInterfaces(const std::vector<InterfaceConfig> &ifaces,
                    ConfigurationManager *mgr);
  static std::string
  showGifInterfaces(const std::vector<InterfaceConfig> &ifaces,
                    ConfigurationManager *mgr);
  static std::string
  showOvpnInterfaces(const std::vector<InterfaceConfig> &ifaces,
                     ConfigurationManager *mgr);
  static std::string
  showIpsecInterfaces(const std::vector<InterfaceConfig> &ifaces,
                      ConfigurationManager *mgr);
  static std::string
  showGreInterfaces(const std::vector<InterfaceConfig> &ifaces,
                    ConfigurationManager *mgr);
  static std::string
  showCarpInterfaces(const std::vector<InterfaceConfig> &ifaces,
                     ConfigurationManager *mgr);
  static std::string
  showVxlanInterfaces(const std::vector<InterfaceConfig> &ifaces,
                      ConfigurationManager *mgr);
  static std::string
  showWlanInterfaces(const std::vector<InterfaceConfig> &ifaces,
                     ConfigurationManager *mgr);
  static std::string
  showEpairInterfaces(const std::vector<InterfaceConfig> &ifaces,
                      ConfigurationManager *mgr);
  static std::string
  showWireGuardInterfaces(const std::vector<InterfaceConfig> &ifaces,
                          ConfigurationManager *mgr);
  static std::string
  showTapInterfaces(const std::vector<InterfaceConfig> &ifaces,
                    ConfigurationManager *mgr);
  static std::string
  showSixToFourInterfaces(const std::vector<InterfaceConfig> &ifaces,
                          ConfigurationManager *mgr);
  static std::string
  showPflogInterfaces(const std::vector<InterfaceConfig> &ifaces,
                      ConfigurationManager *mgr);
  static std::string
  showPfsyncInterfaces(const std::vector<InterfaceConfig> &ifaces,
                       ConfigurationManager *mgr);
  static std::string
  showLoopbackInterfaces(const std::vector<InterfaceConfig> &ifaces,
                         ConfigurationManager *mgr);

private:
  InterfaceType type_ = InterfaceType::Unknown;
  std::string name_;
  bool expect_type_value_ = false;
};
