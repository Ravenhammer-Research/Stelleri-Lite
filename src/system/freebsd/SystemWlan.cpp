/*
 * WLAN system helper implementations
 */

#include "Socket.hpp"
#include "SystemConfigurationManager.hpp"
#include "WlanAuthMode.hpp"
#include "WlanInterfaceConfig.hpp"

#include <cstring>
#include <net/if.h>
#include <net/if_media.h>
#include <net80211/_ieee80211.h>
#include <net80211/ieee80211.h>
#include <net80211/ieee80211_crypto.h>
#include <net80211/ieee80211_ioctl.h>
#include <sys/ioctl.h>
#include <sys/sockio.h>
#include <vector>

namespace {

// Populate wireless-specific metadata on a WlanInterfaceConfig.
// Val-type queries:  kernel returns result in req.i_val  (set i_len=0, i_data=nullptr)
// Buffer-type queries: kernel fills req.i_data buffer    (set i_len=bufsize, i_data=buf)
void populateWlanMetadata(WlanInterfaceConfig &w, const std::string &ifname,
                          std::optional<uint32_t> flags) {
  try {
    Socket s(AF_INET, SOCK_DGRAM);
    struct ieee80211req req{};
    std::strncpy(req.i_name, ifname.c_str(), IFNAMSIZ - 1);

    // SSID (buffer-type)
    {
      std::vector<char> buf(256);
      req.i_type = IEEE80211_IOC_SSID;
      req.i_len = static_cast<int>(buf.size());
      req.i_data = buf.data();
      if (ioctl(s, SIOCG80211, &req) == 0 && req.i_len > 0)
        w.ssid = std::string(buf.data(), static_cast<size_t>(req.i_len));
    }

    // Channel — prefer CURCHAN (buffer-type struct ieee80211_channel),
    //           fall back to CHANNEL (val-type).
    {
      struct ieee80211_channel curchan{};
      req.i_type = IEEE80211_IOC_CURCHAN;
      req.i_len = sizeof(curchan);
      req.i_data = &curchan;
      if (ioctl(s, SIOCG80211, &req) == 0) {
        w.channel = curchan.ic_ieee;
        if (curchan.ic_freq)
          w.channel_freq = curchan.ic_freq;
      } else {
        // Fallback: CHANNEL is a val-type query
        req.i_type = IEEE80211_IOC_CHANNEL;
        req.i_len = 0;
        req.i_data = nullptr;
        if (ioctl(s, SIOCG80211, &req) == 0)
          w.channel = req.i_val;
      }
    }

    // BSSID (buffer-type)
    {
      std::vector<unsigned char> mac(8);
      req.i_type = IEEE80211_IOC_BSSID;
      req.i_len = static_cast<int>(mac.size());
      req.i_data = mac.data();
      if (ioctl(s, SIOCG80211, &req) == 0 && req.i_len >= 6) {
        char macbuf[32];
        std::snprintf(macbuf, sizeof(macbuf),
                      "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1],
                      mac[2], mac[3], mac[4], mac[5]);
        w.bssid = std::string(macbuf);
      }
    }

    // Parent device (buffer-type)
    {
      std::vector<char> pbuf(64);
      req.i_type = IEEE80211_IOC_IC_NAME;
      req.i_len = static_cast<int>(pbuf.size());
      req.i_data = pbuf.data();
      if (ioctl(s, SIOCG80211, &req) == 0 && req.i_len > 0)
        w.parent = std::string(pbuf.data(), static_cast<size_t>(req.i_len));
    }

    // Auth mode (val-type) — values match IEEE80211_AUTH_* from <net80211/_ieee80211.h>
    {
      req.i_type = IEEE80211_IOC_AUTHMODE;
      req.i_len = 0;
      req.i_data = nullptr;
      if (ioctl(s, SIOCG80211, &req) == 0)
        w.authmode = static_cast<WlanAuthMode>(req.i_val);
    }

    // WPA version (val-type): 0=none, 1=WPA, 2=WPA2/RSN, 3=both
    {
      req.i_type = IEEE80211_IOC_WPA;
      req.i_len = 0;
      req.i_data = nullptr;
      if (ioctl(s, SIOCG80211, &req) == 0)
        w.wpa_version = req.i_val;
    }

    // Transmit power (val-type) — returned in half-dBm units, convert to dBm
    {
      req.i_type = IEEE80211_IOC_TXPOWER;
      req.i_len = 0;
      req.i_data = nullptr;
      if (ioctl(s, SIOCG80211, &req) == 0)
        w.txpower = req.i_val / 2;
    }

    // Privacy (val-type) — uses IEEE80211_IOC_WEP (OFF=0, ON=1, MIXED=2)
    {
      req.i_type = IEEE80211_IOC_WEP;
      req.i_len = 0;
      req.i_data = nullptr;
      if (ioctl(s, SIOCG80211, &req) == 0)
        w.privacy = (req.i_val != 0);
    }

    // Roaming mode (val-type): DEVICE=0, AUTO=1, MANUAL=2
    {
      req.i_type = IEEE80211_IOC_ROAMING;
      req.i_len = 0;
      req.i_data = nullptr;
      if (ioctl(s, SIOCG80211, &req) == 0)
        w.roaming = static_cast<WlanRoaming>(req.i_val);
    }

    // Status
    if (flags && (*flags & IFF_RUNNING))
      w.status = w.ssid ? "associated" : "up";
    else
      w.status = "down";

    // Driver / HT / VHT capabilities via IEEE80211_IOC_DEVCAPS (buffer-type).
    // We only need the caps words, not the full channel list, so ask for 1 chan.
    {
      size_t sz = IEEE80211_DEVCAPS_SIZE(1);
      std::vector<char> buf(sz, 0);
      auto *dc = reinterpret_cast<struct ieee80211_devcaps_req *>(buf.data());
      dc->dc_chaninfo.ic_nchans = 1;
      req.i_type = IEEE80211_IOC_DEVCAPS;
      req.i_len = static_cast<int>(sz);
      req.i_data = dc;
      if (ioctl(s, SIOCG80211, &req) == 0) {
        w.drivercaps = dc->dc_drivercaps;
        w.htcaps = dc->dc_htcaps;
        w.vhtcaps = dc->dc_vhtcaps;
      }
    }

    // Regulatory domain (buffer-type) — returns struct ieee80211_regdomain
    {
      struct ieee80211_regdomain rd{};
      req.i_type = IEEE80211_IOC_REGDOMAIN;
      req.i_len = sizeof(rd);
      req.i_data = &rd;
      if (ioctl(s, SIOCG80211, &req) == 0) {
        if (rd.isocc[0] && rd.isocc[1])
          w.country = std::string(rd.isocc, 2);
        // Map well-known SKUs to names
        switch (rd.regdomain) {
        case 0x10: w.regdomain = "FCC"; break;
        case 0x20: w.regdomain = "DOC"; break;
        case 0x30: w.regdomain = "ETSI"; break;
        case 0x31: w.regdomain = "SPAIN"; break;
        case 0x32: w.regdomain = "FRANCE"; break;
        case 0x40: w.regdomain = "MKK"; break;
        case 0x41: w.regdomain = "MKK2"; break;
        default:
          if (rd.regdomain)
            w.regdomain = std::to_string(rd.regdomain);
          break;
        }
      }
    }

    // Default TX key (val-type)
    {
      req.i_type = IEEE80211_IOC_WEPTXKEY;
      req.i_len = 0;
      req.i_data = nullptr;
      if (ioctl(s, SIOCG80211, &req) == 0)
        w.deftxkey = (req.i_val == -1) ? -1 : req.i_val + 1;
    }

    // Active cipher — query the pairwise (unicast) WPA key
    {
      struct ieee80211req_key ik{};
      ik.ik_keyix = 0x7fff; // IEEE80211_KEYIX_NONE — unicast key
      // For STA mode, set macaddr to BSSID to get the pairwise key
      if (w.bssid) {
        unsigned int m[6];
        if (std::sscanf(w.bssid->c_str(), "%x:%x:%x:%x:%x:%x",
                        &m[0], &m[1], &m[2], &m[3], &m[4], &m[5]) == 6)
          for (int j = 0; j < 6; ++j)
            ik.ik_macaddr[j] = static_cast<uint8_t>(m[j]);
      }
      req.i_type = IEEE80211_IOC_WPAKEY;
      req.i_len = sizeof(ik);
      req.i_data = &ik;
      if (ioctl(s, SIOCG80211, &req) == 0 && ik.ik_keylen != 0) {
        w.cipher = static_cast<WlanCipher>(ik.ik_type);
        w.cipher_keylen = ik.ik_keylen * 8;
      }
    }

    // Beacon miss threshold (val-type)
    {
      req.i_type = IEEE80211_IOC_BMISSTHRESHOLD;
      req.i_len = 0;
      req.i_data = nullptr;
      if (ioctl(s, SIOCG80211, &req) == 0)
        w.bmiss = req.i_val;
    }

    // Scan cache valid threshold (val-type)
    {
      req.i_type = IEEE80211_IOC_SCANVALID;
      req.i_len = 0;
      req.i_data = nullptr;
      if (ioctl(s, SIOCG80211, &req) == 0)
        w.scanvalid = req.i_val;
    }

    // TX parameters: mcastrate, mgmtrate, maxretry (buffer-type)
    // We need the current channel's mode to index into the params array.
    {
      struct ieee80211_txparams_req tp{};
      req.i_type = IEEE80211_IOC_TXPARAMS;
      req.i_len = sizeof(tp);
      req.i_data = &tp;
      if (ioctl(s, SIOCG80211, &req) == 0) {
        // Determine mode index from media_mode (already populated)
        int mi = IEEE80211_MODE_AUTO;
        if (w.media_mode) {
          switch (*w.media_mode) {
          case WlanMediaMode::A_11A:   mi = IEEE80211_MODE_11A; break;
          case WlanMediaMode::B_11B:   mi = IEEE80211_MODE_11B; break;
          case WlanMediaMode::G_11G:   mi = IEEE80211_MODE_11G; break;
          case WlanMediaMode::NA_11NA: mi = IEEE80211_MODE_11NA; break;
          case WlanMediaMode::NG_11NG: mi = IEEE80211_MODE_11NG; break;
          case WlanMediaMode::VHT_5G:  mi = IEEE80211_MODE_VHT_5GHZ; break;
          case WlanMediaMode::VHT_2G:  mi = IEEE80211_MODE_VHT_2GHZ; break;
          default: break;
          }
        }
        auto &p = tp.params[mi];
        w.mcastrate = p.mcastrate;
        w.mgmtrate = p.mgmtrate;
        w.maxretry = p.maxretry;
      }
    }

    // HT config (val-type): 0=off, 1=HT20, 2=HT40rx, 3=HT20+40
    {
      req.i_type = IEEE80211_IOC_HTCONF;
      req.i_len = 0;
      req.i_data = nullptr;
      if (ioctl(s, SIOCG80211, &req) == 0)
        w.htconf = req.i_val;
    }

    // A-MPDU (val-type): 0=off, 1=tx, 2=rx, 3=both
    {
      req.i_type = IEEE80211_IOC_AMPDU;
      req.i_len = 0;
      req.i_data = nullptr;
      if (ioctl(s, SIOCG80211, &req) == 0)
        w.ampdu = req.i_val;
    }

    // A-MPDU length limit (val-type): 0=8k, 1=16k, 2=32k, 3=64k
    {
      req.i_type = IEEE80211_IOC_AMPDU_LIMIT;
      req.i_len = 0;
      req.i_data = nullptr;
      if (ioctl(s, SIOCG80211, &req) == 0)
        w.ampdu_limit = req.i_val;
    }

    // A-MPDU density (val-type): 0-7
    {
      req.i_type = IEEE80211_IOC_AMPDU_DENSITY;
      req.i_len = 0;
      req.i_data = nullptr;
      if (ioctl(s, SIOCG80211, &req) == 0)
        w.ampdu_density = req.i_val;
    }

    // A-MSDU (val-type): 0=off, 1=tx, 2=rx, 3=both
    {
      req.i_type = IEEE80211_IOC_AMSDU;
      req.i_len = 0;
      req.i_data = nullptr;
      if (ioctl(s, SIOCG80211, &req) == 0)
        w.amsdu = req.i_val;
    }

    // Short guard interval (val-type)
    {
      req.i_type = IEEE80211_IOC_SHORTGI;
      req.i_len = 0;
      req.i_data = nullptr;
      if (ioctl(s, SIOCG80211, &req) == 0)
        w.shortgi = (req.i_val != 0);
    }

    // STBC (val-type): 0=off, 1=tx, 2=rx, 3=both
    {
      req.i_type = IEEE80211_IOC_STBC;
      req.i_len = 0;
      req.i_data = nullptr;
      if (ioctl(s, SIOCG80211, &req) == 0)
        w.stbc = req.i_val;
    }

    // LDPC (val-type): 0=off, 1=tx, 2=rx, 3=both
    {
      req.i_type = IEEE80211_IOC_LDPC;
      req.i_len = 0;
      req.i_data = nullptr;
      if (ioctl(s, SIOCG80211, &req) == 0)
        w.ldpc = req.i_val;
    }

    // U-APSD (val-type)
    {
      req.i_type = IEEE80211_IOC_UAPSD;
      req.i_len = 0;
      req.i_data = nullptr;
      if (ioctl(s, SIOCG80211, &req) == 0)
        w.uapsd = (req.i_val != 0);
    }

    // WME (val-type)
    {
      req.i_type = IEEE80211_IOC_WME;
      req.i_len = 0;
      req.i_data = nullptr;
      if (ioctl(s, SIOCG80211, &req) == 0)
        w.wme = (req.i_val != 0);
    }

    // Media via SIOCGIFMEDIA — store as enum pair, not string
    {
      struct ifmediareq ifmr{};
      std::strncpy(ifmr.ifm_name, ifname.c_str(), IFNAMSIZ - 1);
      if (ioctl(s, SIOCGIFMEDIA, &ifmr) == 0) {
        int media = ifmr.ifm_current;

        // Sub-type (IFM_TMASK = 0x1f)
        int sub = media & IFM_TMASK;
        if (sub == IFM_IEEE80211_MCS)
          w.media_subtype = WlanMediaSubtype::MCS;
        else if (sub == IFM_IEEE80211_VHT)
          w.media_subtype = WlanMediaSubtype::VHT;
        else
          w.media_subtype = WlanMediaSubtype::AUTO;

        // Mode bits (wider mask to capture VHT2G = 0x00080000)
        int mode = media & 0x000f0000;
        if (mode == IFM_IEEE80211_11A)        w.media_mode = WlanMediaMode::A_11A;
        else if (mode == IFM_IEEE80211_11B)   w.media_mode = WlanMediaMode::B_11B;
        else if (mode == IFM_IEEE80211_11G)   w.media_mode = WlanMediaMode::G_11G;
        else if (mode == IFM_IEEE80211_11NA)  w.media_mode = WlanMediaMode::NA_11NA;
        else if (mode == IFM_IEEE80211_11NG)  w.media_mode = WlanMediaMode::NG_11NG;
        else if (mode == IFM_IEEE80211_VHT5G) w.media_mode = WlanMediaMode::VHT_5G;
        else if (mode == IFM_IEEE80211_VHT2G) w.media_mode = WlanMediaMode::VHT_2G;
        else                                  w.media_mode = WlanMediaMode::AUTO;
      }
    }
  } catch (...) {
  }
}

} // anonymous namespace

void SystemConfigurationManager::CreateWlan(const std::string &name) const {
  if (InterfaceConfig::exists(*this, name))
    return;
  cloneInterface(name, SIOCIFCREATE);
}

void SystemConfigurationManager::SaveWlan(
    const WlanInterfaceConfig &wlan) const {
  if (wlan.name.empty())
    throw std::runtime_error("WlanInterfaceConfig has no interface name set");

  if (!InterfaceConfig::exists(*this, wlan.name))
    CreateWlan(wlan.name);

  // Reuse generic interface save for addresses/mtu/flags
  SaveInterface(wlan);
}

std::vector<WlanInterfaceConfig>
SystemConfigurationManager::GetWlanInterfaces(
    const std::optional<VRFConfig> &vrf) const {
  auto bases = GetInterfaces(vrf);
  std::vector<WlanInterfaceConfig> out;
  for (const auto &ic : bases) {
    if (ic.type != InterfaceType::Wireless)
      continue;
    WlanInterfaceConfig w(ic);
    populateWlanMetadata(w, ic.name, ic.flags);
    out.emplace_back(std::move(w));
  }
  return out;
}
