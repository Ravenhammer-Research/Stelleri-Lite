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

#include "CLI.hpp"
#include "CommandGenerator.hpp"
#include "Logger.hpp"
#include "SystemConfigurationManager.hpp"
#ifdef STELLERI_NETCONF
#include "Client.hpp"
#include "NetconfConfigurationManager.hpp"
#include <libnetconf2/log.h>
#endif
#ifdef STELLERI_NETCONF
#include <libnetconf2/netconf.h>
#endif
#include <getopt.h>
#include <iostream>
#include <string>
#include <unistd.h>

int main(int argc, char *argv[]) {
  std::string onecmd;
  bool generate = false;
  bool verbose = false;
#ifdef STELLERI_NETCONF
  const std::string default_unix_socket = "/var/run/stelleri/netconf.sock";
  bool client_initialized = false;
#endif

  struct option longopts[] = {{"file", required_argument, nullptr, 'f'},
                              {"generate", no_argument, nullptr, 'g'},
                              {"interactive", no_argument, nullptr, 'i'},
                              {"verbose", no_argument, nullptr, 'v'},
                              {"help", no_argument, nullptr, 'h'},
#ifdef STELLERI_NETCONF
                              {"unix", required_argument, nullptr, 'U'},
                              {"ssh", required_argument, nullptr, 0},
                              {"tls", required_argument, nullptr, 0},
#endif
                              {0, 0, 0, 0}};

  int ch;
  int longidx = 0;
  while ((ch = getopt_long(argc, argv, "f:givhU:", longopts, &longidx)) != -1) {
    if (ch == 0) {
#ifdef STELLERI_NETCONF
      const char *lname = longopts[longidx].name;
      if (lname) {
        if (std::strcmp(lname, "ssh") == 0) {
#ifdef NC_ENABLED_SSH_TLS
          std::string s(optarg);
          auto p = s.find(':');
          std::string host;
          uint16_t port;
          if (p == std::string::npos) {
            host = s;
            port = NC_PORT_SSH;
          } else {
            host = s.substr(0, p);
            port = static_cast<uint16_t>(std::stoi(s.substr(p + 1)));
          }
          if (!Client::init_ssh(host, port)) {
            std::cerr << "netcli: failed to init netconf ssh client: " << host
                      << ":" << port << "\n";
          }
#else
          std::cerr << "netcli: ssh support not available in libnetconf2\n";
#endif
        } else if (std::strcmp(lname, "tls") == 0) {
#ifdef NC_ENABLED_SSH_TLS
          std::string s(optarg);
          auto p = s.find(':');
          std::string host;
          uint16_t port;
          if (p == std::string::npos) {
            host = s;
            port = NC_PORT_TLS;
          } else {
            host = s.substr(0, p);
            port = static_cast<uint16_t>(std::stoi(s.substr(p + 1)));
          }
          if (!Client::init_tls(host, port)) {
            std::cerr << "netcli: failed to init netconf tls client: " << host
                      << ":" << port << "\n";
          }
#else
          std::cerr << "netcli: tls support not available in libnetconf2\n";
#endif
        }
      }
#endif
      continue;
    }

    switch (ch) {
    case 'g':
      generate = true;
      break;
    case 'i':
      // Interactive mode (default anyway)
      break;
    case 'v':
      verbose = true;
      break;
#ifdef STELLERI_NETCONF
    case 'U':
      // --unix / -U: initialize unix socket client
      if (!Client::init_unix(optarg)) {
        std::cerr << "netcli: failed to init netconf client (unix): " << optarg
                  << "\n";
      } else {
        client_initialized = true;
      }
      break;
#endif
    case 'h':
      std::cout << "Usage: netcli [command] [-g] [-i] [-v]\n";
      std::cout << "  command           Execute a single command (any non-flag "
                   "args)\n";
      std::cout << "  -g, --generate    Generate configuration from system\n";
      std::cout << "  -i, --interactive Enter interactive mode\n";
      std::cout << "  -v, --verbose     Enable verbose output\n";
      std::cout << "  -h, --help        Show this help message\n";
      std::cout << "Netconf options (STELLERI=netconf):\n";
      std::cout << "  -U, --unix PATH           Use unix socket PATH for "
                   "NETCONF client\n";
      std::cout << "  --ssh HOST[:PORT]         Connect NETCONF over SSH\n";
      std::cout << "  --tls HOST[:PORT]         Connect NETCONF over TLS\n";
      return 0;
    default:
      break;
    }
  }

  if (verbose) {
    logger::get().setLevel(logger::Level::Debug);
#ifdef STELLERI_NETCONF
    nc_verbosity(NC_VERB_DEBUG);
#endif
  }

  if (generate) {
    SystemConfigurationManager mgr;
    netcli::CommandGenerator generator;
    generator.generateConfiguration(mgr);
    return 0;
  }

#ifdef STELLERI_NETCONF
  auto mgr = std::make_unique<NetconfConfigurationManager>();
#else
  auto mgr = std::make_unique<SystemConfigurationManager>();
#endif
  CLI cli(std::move(mgr));

#ifdef STELLERI_NETCONF
  if (!client_initialized) {
    // Default the client to the well-known unix socket if available
    Client::init_unix(default_unix_socket);
  }
#endif

  // If there are non-option arguments left, treat them as a single one-shot
  // command (join remaining argv entries with spaces).
  if (optind < argc) {
    std::string cmd;
    for (int i = optind; i < argc; ++i) {
      if (!cmd.empty())
        cmd += ' ';
      cmd += argv[i];
    }
    if (!cmd.empty()) {
      cli.processLine(cmd);
      return 0;
    }
  }

  // Check if STDIN is redirected (pipe or file)
  if (!isatty(STDIN_FILENO)) {
    // Read commands from STDIN
    std::string line;
    while (std::getline(std::cin, line)) {
      if (line.empty() || line[0] == '#') {
        // Skip empty lines and comments
        continue;
      }
      cli.processLine(line);
    }
    return 0;
  }

  cli.run();
  return 0;
}
