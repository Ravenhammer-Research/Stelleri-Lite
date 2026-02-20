#include "RoutingSocket.hpp"

#include "Socket.hpp"
#include <cerrno>
#include <cstring>
#include <sys/socket.h>
#include <unistd.h>

bool WriteRoutingSocket(const std::vector<char> &msg, int proto) {
  try {
    Socket sock(PF_ROUTE, SOCK_RAW, proto);
    ssize_t written = ::write(sock.fd(), msg.data(), msg.size());
    return written == static_cast<ssize_t>(msg.size());
  } catch (const SocketException &) {
    return false;
  }
}
