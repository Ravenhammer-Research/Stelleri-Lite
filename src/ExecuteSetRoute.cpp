// ExecuteSetRoute.cpp
// Implement RTM_ADD via routing socket (pack rt_msghdr + sockaddrs)

#include "ConfigurationManager.hpp"
#include "Parser.hpp"
#include "RouteConfig.hpp"
#include "RouteToken.hpp"
#include "IPNetwork.hpp"

#include <sys/types.h>
#include <sys/socket.h>
#include <net/route.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <cstring>
#include <iostream>

void netcli::Parser::executeSetRoute(const RouteToken &tok,
																		 ConfigurationManager *mgr) const {
	(void)mgr;
	RouteConfig rc;
	rc.prefix = tok.prefix();
	if (tok.nexthop)
		rc.nexthop = tok.nexthop->toString();
	if (tok.interface)
		rc.iface = tok.interface->name();
	if (tok.vrf)
		rc.vrf = tok.vrf->name();
	rc.blackhole = tok.blackhole;
	rc.reject = tok.reject;
	rc.save();

	auto net = IPNetwork::fromString(rc.prefix);
	if (!net) {
		std::cout << "set route: invalid prefix: " << rc.prefix << "\n";
		return;
	}

	// prepare routing socket
	int s = socket(PF_ROUTE, SOCK_RAW, 0);
	if (s < 0) {
		std::cout << "set route: open routing socket failed: " << strerror(errno)
							<< "\n";
		return;
	}
    

	// If VRF/ FIB numeric provided, attempt to set socket option SO_SETFIB
	if (rc.vrf) {
		std::string v = *rc.vrf;
		int fib = -1;
		if (v.rfind("fib", 0) == 0) {
			try { fib = std::stoi(v.substr(3)); } catch (...) { fib = -1; }
		} else {
			try { fib = std::stoi(v); } catch (...) { fib = -1; }
		}
		if (fib >= 0) {
			int so_rc = setsockopt(s, SOL_SOCKET, SO_SETFIB, &fib, sizeof(fib));
			std::cerr << "[dbg] setsockopt(SO_SETFIB, " << fib << ") rc=" << so_rc
								<< " errno=" << errno << "\n";
		}
	}

	// Build message buffer similar to route.c's rtmsg_rtsock
	struct {
		struct rt_msghdr m_rtm;
		char m_space[512];
	} m_rtmsg;
	memset(&m_rtmsg, 0, sizeof(m_rtmsg));

	struct rt_msghdr *rtm = &m_rtmsg.m_rtm;
	char *cp = m_rtmsg.m_space;

	rtm->rtm_version = RTM_VERSION;
	rtm->rtm_type = RTM_ADD;
	rtm->rtm_seq = 1;
	rtm->rtm_pid = getpid();

            
	rtm->rtm_flags = RTF_UP | RTF_STATIC;
	if (rc.blackhole)
		rtm->rtm_flags |= RTF_BLACKHOLE;
	if (rc.reject)
		rtm->rtm_flags |= RTF_REJECT;

	// DST
	if (net->family() == AddressFamily::IPv4) {
		rtm->rtm_addrs |= RTA_DST;
		struct sockaddr_in sin_dst{};
		sin_dst.sin_len = sizeof(sin_dst);
		sin_dst.sin_family = AF_INET;
		std::string dst = net->address()->toString();
		inet_pton(AF_INET, dst.c_str(), &sin_dst.sin_addr);
		memcpy(cp, &sin_dst, sizeof(sin_dst));
		cp += sizeof(sin_dst);

		// GATEWAY (optional)
		if (rc.nexthop) {
			rtm->rtm_addrs |= RTA_GATEWAY;
			struct sockaddr_in sin_gw{};
			sin_gw.sin_len = sizeof(sin_gw);
			sin_gw.sin_family = AF_INET;
			inet_pton(AF_INET, rc.nexthop->c_str(), &sin_gw.sin_addr);
			rtm->rtm_flags |= RTF_GATEWAY;
			memcpy(cp, &sin_gw, sizeof(sin_gw));
			cp += sizeof(sin_gw);
		}

		// NETMASK
		if (net->mask() < 32) {
			rtm->rtm_addrs |= RTA_NETMASK;
			struct sockaddr_in sin_mask{};
			sin_mask.sin_len = sizeof(sin_mask);
			sin_mask.sin_family = AF_INET;
			auto m = IPAddress::maskFromCIDR(AddressFamily::IPv4, net->mask());
			if (m) {
				inet_pton(AF_INET, m->toString().c_str(), &sin_mask.sin_addr);
			}
			memcpy(cp, &sin_mask, sizeof(sin_mask));
			cp += sizeof(sin_mask);
		}
	} else if (net->family() == AddressFamily::IPv6) {
		rtm->rtm_addrs |= RTA_DST;
		struct sockaddr_in6 sin6_dst{};
		sin6_dst.sin6_len = sizeof(sin6_dst);
		sin6_dst.sin6_family = AF_INET6;
		std::string dst = net->address()->toString();
		inet_pton(AF_INET6, dst.c_str(), &sin6_dst.sin6_addr);
		memcpy(cp, &sin6_dst, sizeof(sin6_dst));
		cp += sizeof(sin6_dst);

		if (rc.nexthop) {
			rtm->rtm_addrs |= RTA_GATEWAY;
			struct sockaddr_in6 sin6_gw{};
			sin6_gw.sin6_len = sizeof(sin6_gw);
			sin6_gw.sin6_family = AF_INET6;
			inet_pton(AF_INET6, rc.nexthop->c_str(), &sin6_gw.sin6_addr);
			rtm->rtm_flags |= RTF_GATEWAY;
			memcpy(cp, &sin6_gw, sizeof(sin6_gw));
			cp += sizeof(sin6_gw);
		}

		if (net->mask() < 128) {
			rtm->rtm_addrs |= RTA_NETMASK;
			struct sockaddr_in6 sin6_mask{};
			sin6_mask.sin6_len = sizeof(sin6_mask);
			sin6_mask.sin6_family = AF_INET6;
			auto m6 = IPAddress::maskFromCIDR(AddressFamily::IPv6, net->mask());
			if (m6) {
				inet_pton(AF_INET6, m6->toString().c_str(), &sin6_mask.sin6_addr);
			}
			memcpy(cp, &sin6_mask, sizeof(sin6_mask));
			cp += sizeof(sin6_mask);
		}
	}

	rtm->rtm_msglen = static_cast<u_short>(cp - reinterpret_cast<char *>(&m_rtmsg));

    

	ssize_t w = write(s, &m_rtmsg, rtm->rtm_msglen);
	if (w == -1) {
		std::cout << "set route: write to routing socket failed: " << strerror(errno)
							<< "\n";
		close(s);
		return;
	}

	std::cout << "set route: " << rc.prefix << " added\n";
	close(s);
}

