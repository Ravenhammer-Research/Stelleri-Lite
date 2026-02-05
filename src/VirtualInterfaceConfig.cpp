#include "VirtualInterfaceConfig.hpp"

#include <cerrno>
#include <cstring>
#include <iostream>
#include <net/if.h>
#include <net/if_clone.h>
#include <stdexcept>
#include <string>
#include <sys/ioctl.h>
#include <sys/linker.h>
#include <sys/socket.h>
#include <unistd.h>

VirtualInterfaceConfig::VirtualInterfaceConfig(const InterfaceConfig &base) {
  name = base.name;
  type = base.type;
  if (base.address)
    address = base.address->clone();
  aliases.clear();
  for (const auto &a : base.aliases) {
    if (a)
      aliases.push_back(a->clone());
  }
  if (base.vrf)
    vrf = std::make_unique<VRFConfig>(*base.vrf);
  flags = base.flags;
  tunnel_vrf = base.tunnel_vrf;
  groups = base.groups;
  mtu = base.mtu;
}

VirtualInterfaceConfig::VirtualInterfaceConfig(const InterfaceConfig &base,
                                               std::optional<std::string> peer_,
                                               std::optional<int> rdomain_,
                                               bool promiscuous_) {
  name = base.name;
  type = base.type;
  if (base.address)
    address = base.address->clone();
  aliases.clear();
  for (const auto &a : base.aliases) {
    if (a)
      aliases.push_back(a->clone());
  }
  if (base.vrf)
    vrf = std::make_unique<VRFConfig>(*base.vrf);
  flags = base.flags;
  tunnel_vrf = base.tunnel_vrf;
  groups = base.groups;
  mtu = base.mtu;

  peer = peer_;
  rdomain = rdomain_;
  promiscuous = promiscuous_;
}

void VirtualInterfaceConfig::save() const {
  if (!InterfaceConfig::exists(name)) {
    create();
  } else {
    InterfaceConfig::save();
  }

  if (promiscuous) {
    // Promiscuous mode note intentionally silent by default.
  }
}

void VirtualInterfaceConfig::create() const {
  const std::string &nm = name;
  int sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock < 0) {
    throw std::runtime_error("Failed to create socket: " +
                             std::string(strerror(errno)));
  }

  struct ifreq ifr;
  std::memset(&ifr, 0, sizeof(ifr));
  std::strncpy(ifr.ifr_name, nm.c_str(), IFNAMSIZ - 1);

  if (ioctl(sock, SIOCIFCREATE2, &ifr) < 0) {
    int err = errno;
    std::cerr << "debug: SIOCIFCREATE2('" << nm << "') failed: errno=" << err
              << " (" << strerror(err) << ")\n";
    // Special-case epair clones: if user requested epairNN but kernel rejects
    // that exact name, try creating a generic 'epair' clone and rename the
    // created pair to the requested unit (epairNNa/epairNNb).
    if (err == EINVAL && nm.rfind("epair", 0) == 0) {
      // Attempt generic create with 'epair' base (ifconfig uses SIOCIFCREATE2)
      struct ifreq tmp_ifr;
      std::memset(&tmp_ifr, 0, sizeof(tmp_ifr));
      std::strncpy(tmp_ifr.ifr_name, "epair", IFNAMSIZ - 1);
      if (ioctl(sock, SIOCIFCREATE2, &tmp_ifr) < 0) {
        int e = errno;
        std::cerr << "debug: SIOCIFCREATE2('epair') failed: errno=" << e << " ("
                  << strerror(e) << ")\n";

        // Try to load the if_epair kernel module and retry once.
        auto try_load_and_retry = [&]() -> bool {
          if (kldfind("if_epair.ko") != -1)
            return false; // already found but create failed
          int kid = kldload("if_epair.ko");
          if (kid == -1) {
            std::cerr << "debug: kldload(if_epair.ko) failed: "
                      << strerror(errno) << "\n";
            return false;
          }
          // retry create
          std::memset(&tmp_ifr, 0, sizeof(tmp_ifr));
          std::strncpy(tmp_ifr.ifr_name, "epair", IFNAMSIZ - 1);
          if (ioctl(sock, SIOCIFCREATE2, &tmp_ifr) == 0)
            return true;
          std::cerr
              << "debug: SIOCIFCREATE2('epair') retry after kldload failed: "
              << strerror(errno) << "\n";
          return false;
        };

        if (!try_load_and_retry()) {
          close(sock);
          throw std::runtime_error("Failed to create epair interface: " +
                                   std::string(strerror(e)));
        }
      }

      // tmp_ifr.ifr_name should contain the first created peer, e.g. epair0a
      std::string created = tmp_ifr.ifr_name;
      std::cerr << "debug: created epair peer: " << created << "\n";
      if (created.empty()) {
        close(sock);
        throw std::runtime_error("Failed to determine created epair name");
      }

      // Determine desired target names from requested nm (e.g., epair14)
      // Target suffixes should be 'a' and 'b'. If nm already ends with a/b,
      // use it as-is; otherwise append 'a'/'b'.
      std::string targetBase = nm;
      // If requested name ends with letter a/b, strip it (we'll append later)
      if (!targetBase.empty() &&
          (targetBase.back() == 'a' || targetBase.back() == 'b'))
        targetBase.pop_back();

      std::string srcPeerA = created;
      std::string srcPeerB = created;
      // replace trailing 'a' with 'b' for the peer
      if (!srcPeerA.empty() && srcPeerA.back() == 'a')
        srcPeerB.back() = 'b';
      else {
        // If created doesn't end with 'a', try to derive peer by appending 'b'
        srcPeerB = srcPeerA + "b";
      }

      std::string tgtA = targetBase + "a";
      std::string tgtB = targetBase + "b";

      // Helper to rename an interface using SIOCSIFNAME: set ifr.ifr_name to
      // current name and ifr.ifr_data to the desired new name.
      auto rename_iface = [&](const std::string &cur, const std::string &newn) {
        struct ifreq nr;
        std::memset(&nr, 0, sizeof(nr));
        std::strncpy(nr.ifr_name, cur.c_str(), IFNAMSIZ - 1);
        // pass new name via ifr_data
        nr.ifr_data = const_cast<char *>(newn.c_str());
        if (ioctl(sock, SIOCSIFNAME, &nr) < 0) {
          int e = errno;
          std::cerr << "Warning: failed to rename " << cur << " -> " << newn
                    << ": " << strerror(e) << "\n";
          return false;
        }
        return true;
      };

      // Attempt to rename both peers. If renaming the second fails, try to
      // rename back the first to avoid leaving half-renamed pairs.
      bool okA = rename_iface(srcPeerA, tgtA);
      bool okB = rename_iface(srcPeerB, tgtB);
      if (!okB && okA) {
        // try to revert A
        rename_iface(tgtA, srcPeerA);
      }

      close(sock);
      if (!okA || !okB) {
        throw std::runtime_error("Failed to create/rename epair interfaces");
      }

      return;
    }

    close(sock);
    throw std::runtime_error("Failed to create interface '" + nm +
                             "': " + std::string(strerror(err)));
  }

  close(sock);
}
