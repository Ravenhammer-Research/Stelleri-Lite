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
#include "CommandDispatcher.hpp"
#include "DeleteCommand.hpp"
#include "Parser.hpp"
#include <fstream>
#include <iostream>
#include <sys/stat.h>
#include <unistd.h>

CLI::CLI(std::unique_ptr<ConfigurationManager> mgr) : mgr_(std::move(mgr)) {
  // Set history file to ~/.netcli_history
  const char *home = getenv("HOME");
  if (home) {
    historyFile_ = std::string(home) + "/.netcli_history";
  }
  loadHistory();
}

CLI::~CLI() {}

void CLI::loadHistory() {
  // History is loaded on demand during interactive mode
}

void CLI::saveHistory(const std::string &line) {
  if (historyFile_.empty() || line.empty())
    return;
  std::ofstream hist(historyFile_, std::ios::app);
  if (hist.is_open()) {
    hist << line << '\n';
  }
}

void CLI::processLine(const std::string &line) {
  if (line.empty())
    return;
  if (line == "exit" || line == "quit") {
    std::exit(0);
  }

  // Save to history but do not perform any system changes here — stubbed.
  saveHistory(line);

  netcli::Parser parser;
  auto toks = parser.tokenize(line);
  auto cmd = parser.parse(toks);
  if (!cmd || !cmd->head()) {
    std::cerr << "Error: Invalid command\n";
    return;
  }

  // Dispatch parsed command via CommandDispatcher
  netcli::CommandDispatcher dispatcher;
  dispatcher.dispatch(cmd->head(), mgr_.get());
}

void CLI::run() {
  std::string line;
  while (true) {
    std::cout << "net> " << std::flush;
    if (!std::getline(std::cin, line))
      break; // EOF
    if (line.empty())
      continue;
    processLine(line);
  }
}
