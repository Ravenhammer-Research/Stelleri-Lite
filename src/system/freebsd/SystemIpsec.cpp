/*
 * IPsec system helper implementations
 */

#include "IpsecInterfaceConfig.hpp"
#include "Socket.hpp"
#include "SystemConfigurationManager.hpp"

#include "IPAddress.hpp"

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <net/if.h>
#include <net/if_ipsec.h>
#include <net/pfkeyv2.h>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

// ipsec_set_policy / ipsec_get_policylen / ipsec_strerror from libipsec
extern "C" {
  caddr_t ipsec_set_policy(const char *, int);
  int ipsec_get_policylen(caddr_t);
  char *ipsec_dump_policy(caddr_t, const char *);
  const char *ipsec_strerror(void);
}

// ---------------------------------------------------------------------------
// Helpers: parse hex key string to byte vector, resolve algorithm name to
// SADB constant, build sockaddr from string.
// ---------------------------------------------------------------------------

/// Strip optional leading "0x" / "0X" and decode hex nibbles.
static std::vector<uint8_t> hexToBytes(const std::string &hex) {
  std::string h = hex;
  if (h.size() >= 2 && h[0] == '0' && (h[1] == 'x' || h[1] == 'X'))
    h = h.substr(2);
  std::vector<uint8_t> out;
  out.reserve(h.size() / 2);
  for (size_t i = 0; i + 1 < h.size(); i += 2) {
    auto byte = static_cast<uint8_t>(std::stoul(h.substr(i, 2), nullptr, 16));
    out.push_back(byte);
  }
  return out;
}

/// Map auth algorithm name to SADB_AALG_* constant.
static uint8_t authAlgorithmFromName(const std::string &name) {
  if (name == "hmac-md5")
    return SADB_AALG_MD5HMAC;
  if (name == "hmac-sha1")
    return SADB_AALG_SHA1HMAC;
  if (name == "hmac-sha2-256")
    return SADB_X_AALG_SHA2_256;
  if (name == "hmac-sha2-384")
    return SADB_X_AALG_SHA2_384;
  if (name == "hmac-sha2-512")
    return SADB_X_AALG_SHA2_512;
  if (name == "aes-xcbc-mac")
    return SADB_X_AALG_AES_XCBC_MAC;
  if (name == "null")
    return SADB_X_AALG_NULL;
  throw std::runtime_error("Unknown auth algorithm: " + name);
}

/// Map encryption algorithm name to SADB_EALG_* constant.
static uint8_t encAlgorithmFromName(const std::string &name) {
  if (name == "des-cbc")
    return SADB_EALG_DESCBC;
  if (name == "3des-cbc")
    return SADB_EALG_3DESCBC;
  if (name == "aes-cbc" || name == "aes" || name == "rijndael-cbc")
    return SADB_X_EALG_AESCBC;
  if (name == "aes-ctr")
    return SADB_X_EALG_AESCTR;
  if (name == "blowfish-cbc")
    return SADB_X_EALG_BLOWFISHCBC;
  if (name == "cast128-cbc")
    return SADB_X_EALG_CAST128CBC;
  if (name == "null")
    return SADB_EALG_NULL;
  if (name == "chacha20-poly1305")
    return SADB_X_EALG_CHACHA20POLY1305;
  if (name == "aes-gcm-16")
    return SADB_X_EALG_AESGCM16;
  throw std::runtime_error("Unknown encryption algorithm: " + name);
}

/// Fill a sockaddr_in from a dotted-quad string.
static void fillSockaddrV4(struct sockaddr_in &sin, const std::string &addr) {
  std::memset(&sin, 0, sizeof(sin));
  sin.sin_len = sizeof(sin);
  sin.sin_family = AF_INET;
  if (inet_pton(AF_INET, addr.c_str(), &sin.sin_addr) != 1)
    throw std::runtime_error("Invalid IPv4 address: " + addr);
}

/// Align a byte count up to the next 8-byte boundary (PF_KEY alignment).
static inline size_t pfkeyAlign8(size_t n) {
  return (n + 7) & ~static_cast<size_t>(7);
}

// ---------------------------------------------------------------------------
// Raw PF_KEY SA add.
// Constructs a SADB_ADD message with SA, address-src, address-dst, and
// auth (and optionally encrypt) key extensions, then writes it to a PF_KEY
// socket.
// ---------------------------------------------------------------------------

static void pfkeyAddSA(const IpsecSA &sa) {
  // Determine SA type
  uint8_t satype = 0;
  if (sa.protocol == "ah")
    satype = SADB_SATYPE_AH;
  else if (sa.protocol == "esp")
    satype = SADB_SATYPE_ESP;
  else
    throw std::runtime_error("Unknown SA protocol: " + sa.protocol);

  // Decode keys
  auto authKeyBytes = hexToBytes(sa.auth_key);
  uint8_t authAlg = authAlgorithmFromName(sa.algorithm);

  uint8_t encAlg = SADB_EALG_NONE;
  std::vector<uint8_t> encKeyBytes;
  if (sa.enc_algorithm) {
    encAlg = encAlgorithmFromName(*sa.enc_algorithm);
    if (sa.enc_key)
      encKeyBytes = hexToBytes(*sa.enc_key);
  }

  // Build source / destination sockaddrs
  struct sockaddr_in sa_src, sa_dst;
  fillSockaddrV4(sa_src, sa.src);
  fillSockaddrV4(sa_dst, sa.dst);

  // Compute extension sizes (all in bytes, then / 8 for len fields)
  const size_t addrExtLen =
      pfkeyAlign8(sizeof(struct sadb_address) + sizeof(struct sockaddr_in));
  const size_t authKeyExtLen =
      pfkeyAlign8(sizeof(struct sadb_key) + authKeyBytes.size());
  const size_t encKeyExtLen =
      encKeyBytes.empty()
          ? 0
          : pfkeyAlign8(sizeof(struct sadb_key) + encKeyBytes.size());

  const size_t totalLen = sizeof(struct sadb_msg) + sizeof(struct sadb_sa) +
                          addrExtLen * 2 + authKeyExtLen + encKeyExtLen;

  std::vector<uint8_t> buf(totalLen, 0);
  size_t off = 0;

  // --- sadb_msg header ---
  auto *msg = reinterpret_cast<struct sadb_msg *>(buf.data() + off);
  msg->sadb_msg_version = PF_KEY_V2;
  msg->sadb_msg_type = SADB_ADD;
  msg->sadb_msg_satype = satype;
  msg->sadb_msg_len = static_cast<uint16_t>(totalLen / 8);
  msg->sadb_msg_pid = static_cast<uint32_t>(getpid());
  off += sizeof(struct sadb_msg);

  // --- sadb_sa extension ---
  auto *saExt = reinterpret_cast<struct sadb_sa *>(buf.data() + off);
  saExt->sadb_sa_len = sizeof(struct sadb_sa) / 8;
  saExt->sadb_sa_exttype = SADB_EXT_SA;
  saExt->sadb_sa_spi = htonl(sa.spi);
  saExt->sadb_sa_replay = 0;
  saExt->sadb_sa_state = SADB_SASTATE_MATURE;
  saExt->sadb_sa_auth = authAlg;
  saExt->sadb_sa_encrypt = encAlg;
  off += sizeof(struct sadb_sa);

  // --- address source ---
  auto *srcExt = reinterpret_cast<struct sadb_address *>(buf.data() + off);
  srcExt->sadb_address_len = static_cast<uint16_t>(addrExtLen / 8);
  srcExt->sadb_address_exttype = SADB_EXT_ADDRESS_SRC;
  srcExt->sadb_address_proto = 0;
  srcExt->sadb_address_prefixlen = 32;
  std::memcpy(buf.data() + off + sizeof(struct sadb_address), &sa_src,
              sizeof(sa_src));
  off += addrExtLen;

  // --- address destination ---
  auto *dstExt = reinterpret_cast<struct sadb_address *>(buf.data() + off);
  dstExt->sadb_address_len = static_cast<uint16_t>(addrExtLen / 8);
  dstExt->sadb_address_exttype = SADB_EXT_ADDRESS_DST;
  dstExt->sadb_address_proto = 0;
  dstExt->sadb_address_prefixlen = 32;
  std::memcpy(buf.data() + off + sizeof(struct sadb_address), &sa_dst,
              sizeof(sa_dst));
  off += addrExtLen;

  // --- auth key ---
  auto *akExt = reinterpret_cast<struct sadb_key *>(buf.data() + off);
  akExt->sadb_key_len = static_cast<uint16_t>(authKeyExtLen / 8);
  akExt->sadb_key_exttype = SADB_EXT_KEY_AUTH;
  akExt->sadb_key_bits = static_cast<uint16_t>(authKeyBytes.size() * 8);
  std::memcpy(buf.data() + off + sizeof(struct sadb_key), authKeyBytes.data(),
              authKeyBytes.size());
  off += authKeyExtLen;

  // --- encrypt key (optional, ESP only) ---
  if (!encKeyBytes.empty()) {
    auto *ekExt = reinterpret_cast<struct sadb_key *>(buf.data() + off);
    ekExt->sadb_key_len = static_cast<uint16_t>(encKeyExtLen / 8);
    ekExt->sadb_key_exttype = SADB_EXT_KEY_ENCRYPT;
    ekExt->sadb_key_bits = static_cast<uint16_t>(encKeyBytes.size() * 8);
    std::memcpy(buf.data() + off + sizeof(struct sadb_key), encKeyBytes.data(),
                encKeyBytes.size());
    off += encKeyExtLen;
  }

  // Open PF_KEY socket and send
  int fd = socket(PF_KEY, SOCK_RAW, PF_KEY_V2);
  if (fd < 0)
    throw std::runtime_error("PF_KEY socket(): " +
                             std::string(strerror(errno)));

  ssize_t n = write(fd, buf.data(), totalLen);
  if (n < 0) {
    int err = errno;
    close(fd);
    throw std::runtime_error("PF_KEY SADB_ADD write(): " +
                             std::string(strerror(err)));
  }

  // Read reply to consume the kernel's response
  uint8_t rbuf[4096];
  (void)read(fd, rbuf, sizeof(rbuf));
  close(fd);
}

// ---------------------------------------------------------------------------
// SP add via ipsec_set_policy() + raw PF_KEY SADB_X_SPDADD message.
// ---------------------------------------------------------------------------

static void pfkeyAddSP(const IpsecSP &sp, const std::string &srcAddr,
                       const std::string &dstAddr) {
  // Build the policy buffer using libipsec
  caddr_t polBuf =
      ipsec_set_policy(sp.policy.c_str(), static_cast<int>(sp.policy.size()));
  if (polBuf == nullptr)
    throw std::runtime_error("ipsec_set_policy failed: " +
                             std::string(ipsec_strerror()));

  int polLen = ipsec_get_policylen(polBuf);

  // Determine direction
  uint8_t dir = 0;
  if (sp.direction == "in")
    dir = 1; // IPSEC_DIR_INBOUND
  else if (sp.direction == "out")
    dir = 2; // IPSEC_DIR_OUTBOUND
  else {
    free(polBuf);
    throw std::runtime_error("Unknown SP direction: " + sp.direction);
  }

  // Resolve addresses
  struct sockaddr_in sa_src, sa_dst;
  fillSockaddrV4(sa_src, srcAddr);
  fillSockaddrV4(sa_dst, dstAddr);

  const size_t addrExtLen =
      pfkeyAlign8(sizeof(struct sadb_address) + sizeof(struct sockaddr_in));
  const size_t polExtLen = pfkeyAlign8(static_cast<size_t>(polLen));
  const size_t totalLen = sizeof(struct sadb_msg) + addrExtLen * 2 + polExtLen;

  std::vector<uint8_t> buf(totalLen, 0);
  size_t off = 0;

  // --- sadb_msg header ---
  auto *msg = reinterpret_cast<struct sadb_msg *>(buf.data() + off);
  msg->sadb_msg_version = PF_KEY_V2;
  msg->sadb_msg_type = SADB_X_SPDADD;
  msg->sadb_msg_satype = 0;
  msg->sadb_msg_len = static_cast<uint16_t>(totalLen / 8);
  msg->sadb_msg_pid = static_cast<uint32_t>(getpid());
  off += sizeof(struct sadb_msg);

  // --- address source ---
  auto *srcExt = reinterpret_cast<struct sadb_address *>(buf.data() + off);
  srcExt->sadb_address_len = static_cast<uint16_t>(addrExtLen / 8);
  srcExt->sadb_address_exttype = SADB_EXT_ADDRESS_SRC;
  srcExt->sadb_address_proto = 0; // any protocol
  srcExt->sadb_address_prefixlen = 32;
  std::memcpy(buf.data() + off + sizeof(struct sadb_address), &sa_src,
              sizeof(sa_src));
  off += addrExtLen;

  // --- address destination ---
  auto *dstExt = reinterpret_cast<struct sadb_address *>(buf.data() + off);
  dstExt->sadb_address_len = static_cast<uint16_t>(addrExtLen / 8);
  dstExt->sadb_address_exttype = SADB_EXT_ADDRESS_DST;
  dstExt->sadb_address_proto = 0;
  dstExt->sadb_address_prefixlen = 32;
  std::memcpy(buf.data() + off + sizeof(struct sadb_address), &sa_dst,
              sizeof(sa_dst));
  off += addrExtLen;

  // --- policy extension (built by libipsec) ---
  std::memcpy(buf.data() + off, polBuf, static_cast<size_t>(polLen));
  // The sadb_x_policy already has its len/exttype filled by ipsec_set_policy.
  // Patch the direction field.
  auto *polHdr = reinterpret_cast<struct sadb_x_policy *>(buf.data() + off);
  polHdr->sadb_x_policy_dir = dir;
  off += polExtLen;

  free(polBuf);

  // Open PF_KEY socket and send
  int fd = socket(PF_KEY, SOCK_RAW, PF_KEY_V2);
  if (fd < 0)
    throw std::runtime_error("PF_KEY socket(): " +
                             std::string(strerror(errno)));

  ssize_t n = write(fd, buf.data(), totalLen);
  if (n < 0) {
    int err = errno;
    close(fd);
    throw std::runtime_error("PF_KEY SADB_X_SPDADD write(): " +
                             std::string(strerror(err)));
  }

  uint8_t rbuf[4096];
  (void)read(fd, rbuf, sizeof(rbuf));
  close(fd);
}

// ---------------------------------------------------------------------------
// Set the reqid on an ipsec interface via IPSECSREQID ioctl.
// ---------------------------------------------------------------------------

static void setIpsecReqid(const std::string &ifname, uint32_t reqid) {
  Socket sock(AF_INET, SOCK_DGRAM);
  struct ifreq ifr;
  std::memset(&ifr, 0, sizeof(ifr));
  std::strncpy(ifr.ifr_name, ifname.c_str(), IFNAMSIZ - 1);
  ifr.ifr_data = reinterpret_cast<caddr_t>(&reqid);
  if (ioctl(sock, IPSECSREQID, &ifr) < 0)
    throw std::runtime_error("IPSECSREQID ioctl failed: " +
                             std::string(strerror(errno)));
}

// ---------------------------------------------------------------------------
// SaveIpsec — configure tunnel endpoints, then install SA/SP if present.
// ---------------------------------------------------------------------------

void SystemConfigurationManager::SaveIpsec(
    const IpsecInterfaceConfig &t) const {
  if (t.name.empty())
    throw std::runtime_error("IpsecInterfaceConfig has no interface name set");

  bool needEndpoints = (t.source && t.destination);

  if (!InterfaceConfig::exists(*this, t.name))
    CreateIpsec(t.name);
  else
    SaveInterface(static_cast<const InterfaceConfig &>(t));

  // Configure tunnel endpoints if provided
  if (needEndpoints) {
    auto src_addr = IPAddress::fromString(t.source->toString());
    auto dst_addr = IPAddress::fromString(t.destination->toString());
    if (!src_addr || !dst_addr)
      throw std::runtime_error("Invalid ipsec endpoint addresses");
    if (src_addr->family() != dst_addr->family())
      throw std::runtime_error(
          "Ipsec endpoints must be same address family (both IPv4 or IPv6)");

    Socket tsock(AF_INET, SOCK_DGRAM);

    struct ifaliasreq ifra;
    std::memset(&ifra, 0, sizeof(ifra));
    std::strncpy(ifra.ifra_name, t.name.c_str(), IFNAMSIZ - 1);

    if (src_addr->family() == AddressFamily::IPv4) {
      auto v4src = dynamic_cast<const IPv4Address *>(src_addr.get());
      auto v4dst = dynamic_cast<const IPv4Address *>(dst_addr.get());

      auto *sin_src = reinterpret_cast<struct sockaddr_in *>(&ifra.ifra_addr);
      sin_src->sin_family = AF_INET;
      sin_src->sin_len = sizeof(struct sockaddr_in);
      sin_src->sin_addr.s_addr = htonl(v4src->value());

      auto *sin_dst =
          reinterpret_cast<struct sockaddr_in *>(&ifra.ifra_broadaddr);
      sin_dst->sin_family = AF_INET;
      sin_dst->sin_len = sizeof(struct sockaddr_in);
      sin_dst->sin_addr.s_addr = htonl(v4dst->value());

      if (ioctl(tsock, SIOCSIFPHYADDR, &ifra) < 0)
        throw std::runtime_error("Failed to configure IPSEC endpoints: " +
                                 std::string(strerror(errno)));
    } else {
      std::cerr << "Warning: IPv6 ipsec configuration requires routing "
                   "socket - not fully implemented\n";
    }
  }

  // Set reqid on the interface
  if (t.reqid)
    setIpsecReqid(t.name, *t.reqid);

  // Install Security Associations into the kernel SAD
  for (const auto &sa : t.security_associations)
    pfkeyAddSA(sa);

  // Install Security Policies into the kernel SPD
  std::string srcStr, dstStr;
  if (t.source)
    srcStr = t.source->toString();
  if (t.destination)
    dstStr = t.destination->toString();

  for (const auto &sp : t.security_policies) {
    // Use the tunnel endpoints as the SP selector addresses
    if (srcStr.empty() || dstStr.empty())
      throw std::runtime_error(
          "SP requires tunnel source/destination to be configured");
    pfkeyAddSP(sp, srcStr, dstStr);
  }
}

void SystemConfigurationManager::CreateIpsec(const std::string &nm) const {
  cloneInterface(nm, SIOCIFCREATE);
}

std::vector<IpsecInterfaceConfig>
SystemConfigurationManager::GetIpsecInterfaces(
    const std::vector<InterfaceConfig> &bases) const {
  std::vector<IpsecInterfaceConfig> out;
  for (const auto &ic : bases) {
    if (ic.type == InterfaceType::IPsec)
      out.emplace_back(ic);
  }
  return out;
}
