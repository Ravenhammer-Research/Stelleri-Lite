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

/**
 * @file CLI.hpp
 * @brief Command-line interface implementation
 */

#pragma once

#include "ConfigurationManager.hpp"
#include "Parser.hpp"
#include <histedit.h>
#include <memory>
#include <string>
#include <vector>

/**
 * @brief VyOS-style command-line interface
 *
 * Provides an interactive REPL for network configuration with
 * command history persistence.
 */
class CLI {
public:
  /**
   * @brief Construct CLI with a configuration manager
   * @param mgr Configuration manager implementation
   */
  explicit CLI(std::unique_ptr<ConfigurationManager> mgr);
  ~CLI();

  /**
   * @brief Start interactive REPL loop
   */
  void run();

  /**
   * @brief Process a single command line
   * @param line Command string to execute
   */
  void processLine(const std::string &line);

private:
  std::unique_ptr<ConfigurationManager> mgr_;
  netcli::Parser parser_;
  std::string historyFile_;
  EditLine *el_;
  History *hist_;
  HistEvent ev_;
  std::string preview_;
  int preview_len_ = 0;

  void loadHistory();
  void saveHistory(const std::string &line);
  void setupEditLine();
  void cleanupEditLine();

  // Completion support using Parser and Token infrastructure
  std::vector<std::string> getCompletions(const std::vector<std::string> &tokens,
                                          const std::string &partial) const;
  static unsigned char completeCommand(EditLine *el, int ch);
  static unsigned char showInlinePreview(EditLine *el, int ch);
  static unsigned char handleBackspacePreview(EditLine *el, int ch);
  static unsigned char handleCtrlC(EditLine *el, int ch);
  static unsigned char handleCtrlL(EditLine *el, int ch);
  static unsigned char handleCtrlD(EditLine *el, int ch);
  static unsigned char handleCtrlR(EditLine *el, int ch);
  static char *promptFunc(EditLine *el);
};
