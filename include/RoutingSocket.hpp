// Lightweight helper for sending routing socket messages
#pragma once

#include <vector>

// Write a routing socket message. `proto` may be AF_INET or AF_INET6 when
// callers need protocol-specific sockets; default 0 uses protocol 0.
bool WriteRoutingSocket(const std::vector<char> &msg, int proto = 0);
