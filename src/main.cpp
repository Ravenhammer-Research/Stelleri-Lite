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
#include "GenerateConfig.hpp"
#include "SystemConfigurationManager.hpp"
#include <getopt.h>
#include <iostream>

int main(int argc, char *argv[]) {
  std::string onecmd;
  bool generate = false;

  struct option longopts[] = {{"command", required_argument, nullptr, 'c'},
                              {"file", required_argument, nullptr, 'f'},
                              {"generate", no_argument, nullptr, 'g'},
                              {"interactive", no_argument, nullptr, 'i'},
                              {"help", no_argument, nullptr, 'h'},
                              {0, 0, 0, 0}};

  int ch;
  while ((ch = getopt_long(argc, argv, "c:f:gih", longopts, nullptr)) != -1) {
    switch (ch) {
    case 'c':
      onecmd = optarg;
      break;
    case 'g':
      generate = true;
      break;
    case 'i':
      // Interactive mode (default anyway)
      break;
    case 'h':
      std::cout << "Usage: netcli [-c command] [-g] [-i]\n";
      std::cout << "  -c, --command     Execute a single command\n";
      std::cout << "  -g, --generate    Generate configuration from system\n";
      std::cout << "  -i, --interactive Enter interactive mode\n";
      std::cout << "  -h, --help        Show this help message\n";
      return 0;
    default:
      break;
    }
  }

  if (generate) {
    netcli::generateConfiguration();
    return 0;
  }

  auto mgr = std::make_unique<SystemConfigurationManager>();
  CLI cli(std::move(mgr));

  if (!onecmd.empty()) {
    // simple one-shot
    cli.processLine(onecmd);
    return 0;
  }

  cli.run();
  return 0;
}
