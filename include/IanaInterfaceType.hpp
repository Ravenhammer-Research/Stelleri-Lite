#if !defined(STELLERI_NETCONF) || STELLERI_NETCONF != 1
#error "netconf headers are for the STELLERI_NETCONF build only"
#endif

#include "InterfaceType.hpp"

namespace netconf {

  /// Convert InterfaceType to the IANA `iana-if-type` identity name string.
  inline std::string interfaceTypeToIanaIdentity(InterfaceType t) {
    switch (t) {
    case InterfaceType::Ethernet:
      return "iana-if-type:ethernetCsmacd";
    case InterfaceType::Loopback:
      return "iana-if-type:softwareLoopback";
    case InterfaceType::PPP:
      return "iana-if-type:ppp";
    case InterfaceType::Wireless:
      return "iana-if-type:ieee80211";
    case InterfaceType::Bridge:
      return "iana-if-type:bridge";
    case InterfaceType::Lagg:
      return "iana-if-type:ieee8023adLag";
    case InterfaceType::VLAN:
      return "iana-if-type:l2vlan";
    case InterfaceType::Gif:
      return "iana-tunnel-type:gre";
    case InterfaceType::Tun:
      return "iana-tunnel-type:other";
    case InterfaceType::GRE:
      return "iana-tunnel-type:gre";
    case InterfaceType::VXLAN:
      return "iana-tunnel-type:other";
    case InterfaceType::IPsec:
      return "iana-tunnel-type:ipsectunnelmode";
    case InterfaceType::Epair:
      return "iana-if-type:ethernetCsmacd";
    case InterfaceType::Carp:
      return "iana-if-type:ethernetCsmacd";
    case InterfaceType::Tap:
      return "iana-if-type:ethernetCsmacd";
    case InterfaceType::SixToFour:
      return "iana-tunnel-type:sixtofour";
    case InterfaceType::Ovpn:
      return "iana-tunnel-type:other";
    case InterfaceType::VRF:
      return "ietf-network-instance";
    default:
      return "iana-if-type:other";
    }
  }

  inline InterfaceType ianaIdentityToInterfaceType(std::string id) {
    if (id == "iana-if-type:ethernetCsmacd")
      return InterfaceType::Ethernet;
    else if (id == "iana-if-type:softwareLoopback")
      return InterfaceType::Loopback;
    else if (id == "iana-if-type:ppp")
      return InterfaceType::PPP;
    else if (id == "iana-if-type:ieee80211")
      return InterfaceType::Wireless;
    else if (id == "iana-if-type:bridge")
      return InterfaceType::Bridge;
    else if (id == "iana-if-type:ieee8023adLag")
      return InterfaceType::Lagg;
    else if (id == "iana-if-type:l2vlan")
      return InterfaceType::VLAN;
    else if (id == "iana-tunnel-type:gre")
      return InterfaceType::GRE;
    else if (id == "iana-tunnel-type:ipsectunnelmode")
      return InterfaceType::IPsec;
    else if (id == "iana-tunnel-type:sixtofour")
      return InterfaceType::SixToFour;
    else if (id == "ietf-network-instance")
      return InterfaceType::VRF;
    else
      return InterfaceType::Unknown;
  }
} // namespace netconf
