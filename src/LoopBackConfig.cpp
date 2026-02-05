#include "LoopBackConfig.hpp"
#include <cerrno>
#include <cstring>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <stdexcept>
#include <unistd.h>

void LoopBackConfig::create() const {
	if (InterfaceConfig::exists(name))
		return;

	int sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) {
		throw std::runtime_error("Failed to create socket: " + std::string(strerror(errno)));
	}

	struct ifreq ifr;
	std::memset(&ifr, 0, sizeof(ifr));
	std::strncpy(ifr.ifr_name, name.c_str(), IFNAMSIZ - 1);

	if (ioctl(sock, SIOCIFCREATE, &ifr) < 0) {
		int err = errno;
		close(sock);
		throw std::runtime_error("Failed to create interface '" + name + "': " + std::string(strerror(err)));
	}

	close(sock);
}

void LoopBackConfig::save() const {
	// Ensure interface exists; create if necessary, then apply generic settings
	if (!InterfaceConfig::exists(name)) {
		create();
	}
	InterfaceConfig::save();
}

// Minimal implementation file; constructor is inline in header
