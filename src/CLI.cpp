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
#include "InterfaceToken.hpp"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sys/stat.h>
#include <unistd.h>

#include <signal.h>

// SIGINT handling: install our own handler that sets a flag so the main loop
// can reset editline state without exiting. Keep previous action so we can
// restore it on cleanup.
static struct sigaction g_old_sigint_action;
static volatile sig_atomic_t g_sigint_received = 0;

static void cli_sigint_handler(int) {
  g_sigint_received = 1;
}

CLI::CLI(std::unique_ptr<ConfigurationManager> mgr)
    : mgr_(std::move(mgr)), el_(nullptr), hist_(nullptr) {
  // Set history file to ~/.netcli_history
  const char *home = getenv("HOME");
  if (home) {
    historyFile_ = std::string(home) + "/.netcli_history";
  }
}

CLI::~CLI() { cleanupEditLine(); }

void CLI::loadHistory() {
  if (!hist_ || historyFile_.empty())
    return;
  // Load history from file
  history(hist_, &ev_, H_LOAD, historyFile_.c_str());
}

void CLI::saveHistory(const std::string &line) {
  if (!hist_ || historyFile_.empty() || line.empty())
    return;
  // Add to history and save to file
  history(hist_, &ev_, H_ENTER, line.c_str());
  history(hist_, &ev_, H_SAVE, historyFile_.c_str());
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

char *CLI::promptFunc(EditLine *el) {
  (void)el; // unused
  static char prompt[] = "net> ";
  return prompt;
}

void CLI::setupEditLine() {
  // Initialize editline
  el_ = el_init("net", stdin, stdout, stderr);
  if (!el_) {
    std::cerr << "Failed to initialize editline\n";
    return;
  }

  // Install our SIGINT handler to prevent the default action (process exit)
  // and allow the REPL to clear the line. Save previous action for restore.
  struct sigaction sa;
  sa.sa_handler = cli_sigint_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGINT, &sa, &g_old_sigint_action);

  // Set prompt
  el_set(el_, EL_PROMPT, &CLI::promptFunc);

  // (no EL_RPROMPT - we'll insert preview directly into the edit buffer)

  // Enable editor (emacs mode by default)
  el_set(el_, EL_EDITOR, "emacs");

  // Set up completion callback
  el_set(el_, EL_ADDFN, "ed-complete", "Complete command", &CLI::completeCommand);
  el_set(el_, EL_BIND, "^I", "ed-complete", nullptr); // Tab key

  // Add inline preview on self-insert (show first match in reverse)
  el_set(el_, EL_ADDFN, "ed-show-preview", "Show inline preview",
         &CLI::showInlinePreview);
  const char *printables =
      "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";
  for (const char *p = printables; *p; ++p) {
    char bind[2] = {*p, '\0'};
    el_set(el_, EL_BIND, bind, "ed-show-preview", nullptr);
  }

  // Backspace/delete should also update the inline preview. Bind common
  // backspace control sequences (^H and ^?) to a handler that deletes the
  // previous character and recomputes the preview.
  el_set(el_, EL_ADDFN, "ed-backspace-preview", "Backspace with preview",
         &CLI::handleBackspacePreview);
  el_set(el_, EL_BIND, "^H", "ed-backspace-preview", nullptr);
  el_set(el_, EL_BIND, "^?", "ed-backspace-preview", nullptr);

  // Space must clear any active preview and insert the space character.
  // We bind via the hex escape \x20 since a bare " " confuses el_set.
  el_set(el_, EL_ADDFN, "ed-space-preview", "Space with preview clear",
         &CLI::handleSpace);
  el_set(el_, EL_BIND, "\\x20", "ed-space-preview", nullptr);

    // Control key bindings
    el_set(el_, EL_ADDFN, "ed-ctrlc", "Cancel line (Ctrl-C)", &CLI::handleCtrlC);
    el_set(el_, EL_BIND, "^C", "ed-ctrlc", nullptr);

    el_set(el_, EL_ADDFN, "ed-ctrll", "Clear screen (Ctrl-L)", &CLI::handleCtrlL);
    el_set(el_, EL_BIND, "^L", "ed-ctrll", nullptr);

    el_set(el_, EL_ADDFN, "ed-ctrld", "EOF/Delete (Ctrl-D)", &CLI::handleCtrlD);
    el_set(el_, EL_BIND, "^D", "ed-ctrld", nullptr);

    // Bind Ctrl-R to our reverse-search-like handler (prints recent matching
    // history entries). Avoid depending on a libedit builtin that may not
    // exist on all systems.
    el_set(el_, EL_ADDFN, "ed-ctrl-r", "Reverse search history", &CLI::handleCtrlR);
    el_set(el_, EL_BIND, "^R", "ed-ctrl-r", nullptr);

  // Store this pointer for completion callback
  el_set(el_, EL_CLIENTDATA, this);

  // Initialize history
  hist_ = history_init();
  if (hist_) {
    history(hist_, &ev_, H_SETSIZE, 1000);
    el_set(el_, EL_HIST, history, hist_);
    loadHistory();
  }
}

void CLI::cleanupEditLine() {
  if (hist_) {
    history_end(hist_);
    hist_ = nullptr;
  }
  if (el_) {
    el_end(el_);
    el_ = nullptr;
  }
  // Restore previous SIGINT action
  (void)sigaction(SIGINT, &g_old_sigint_action, nullptr);
}

std::vector<std::string>
CLI::getCompletions(const std::vector<std::string> &tokens,
                    const std::string &partial) const {
  if (tokens.empty()) {
    // At start of line: offer top-level commands
    std::vector<std::string> topLevel = {"show", "set", "delete", "exit", "quit"};
    std::vector<std::string> matches;
    for (const auto &cmd : topLevel) {
      if (cmd.rfind(partial, 0) == 0)
        matches.push_back(cmd);
    }
    return matches;
  }

  // Try to parse the tokens so far to build a token chain
  auto cmd = parser_.parse(tokens);
  if (!cmd || !cmd->head())
    return {};

  // Traverse to the tail of the token chain and call autoComplete
  auto tok = cmd->head();
  while (tok && tok->hasNext())
    tok = tok->getNext();

  if (!tok)
    return {};

  // If the tail token is an InterfaceToken, allow it to query the
  // ConfigurationManager for dynamic completions (names/groups).
  if (auto *itok = dynamic_cast<InterfaceToken *>(tok.get())) {
    return itok->autoCompleteWithManager(tokens, partial, mgr_.get());
  }

  return tok->autoComplete(partial);
}

unsigned char CLI::completeCommand(EditLine *el, int ch) {
  (void)ch; // unused

  // Get CLI instance from client data
  CLI *cli = nullptr;
  el_get(el, EL_CLIENTDATA, &cli);
  if (!cli)
    return CC_ERROR;

  // Clear any active inline preview before completing
  if (cli->preview_len_ > 0) {
    // Move cursor after the preview then delete it (el_deletestr deletes before cursor)
    el_cursor(el, cli->preview_len_);
    el_deletestr(el, cli->preview_len_);
    cli->preview_len_ = 0;
  }

  // Get current line info
  const LineInfo *li = el_line(el);
  if (!li)
    return CC_ERROR;

  // Extract word before cursor
  const char *cursor = li->cursor;
  const char *start = li->buffer;
  const char *word_start = cursor;

  // Find word boundary (space or start of line)
  while (word_start > start && word_start[-1] != ' ')
    word_start--;

  std::string partial(word_start, cursor - word_start);

  // Tokenize everything before the current word
  std::string lineBeforeWord(start, word_start);
  auto tokens = cli->parser_.tokenize(lineBeforeWord);

  auto completions = cli->getCompletions(tokens, partial);

  if (completions.empty()) {
    el_beep(el);
    return CC_REFRESH;
  }

  if (completions.size() == 1) {
    // Single match: complete it
    const std::string &match = completions[0];
    std::string insertion = match.substr(partial.size());
    if (!insertion.empty()) {
      el_insertstr(el, insertion.c_str());
    }
    return CC_REFRESH;
  }

  // Multiple matches: display above prompt
  std::cout << '\n';
  for (const auto &comp : completions) {
    std::cout << comp << "  ";
  }
  std::cout << "\n";

  return CC_REDISPLAY;
}

unsigned char CLI::showInlinePreview(EditLine *el, int ch) {
  // Get CLI instance from client data
  CLI *cli = nullptr;
  el_get(el, EL_CLIENTDATA, &cli);
  if (!cli)
    return CC_ERROR;

  // If a previous preview is present, delete it first (move after it then delete)
  if (cli->preview_len_ > 0) {
    el_cursor(el, cli->preview_len_);
    el_deletestr(el, cli->preview_len_);
    cli->preview_len_ = 0;
  }

  // Insert the typed character at cursor
  char cbuf[2] = {static_cast<char>(ch), '\0'};
  el_insertstr(el, cbuf);

  // Get current line info after insertion
  const LineInfo *li = el_line(el);
  if (!li)
    return CC_REDISPLAY;

  const char *cursor = li->cursor;
  const char *start = li->buffer;
  const char *word_start = cursor;
  while (word_start > start && word_start[-1] != ' ')
    word_start--;

  if (word_start >= cursor)
    return CC_REDISPLAY;

  std::string partial(word_start, cursor - word_start);
  if (partial.empty())
    return CC_REDISPLAY;

  // Tokenize everything before the current word
  std::string lineBeforeWord(start, word_start);
  auto tokens = cli->parser_.tokenize(lineBeforeWord);

  auto completions = cli->getCompletions(tokens, partial);
  if (completions.empty() || completions[0].size() <= partial.size()) {
    return CC_REDISPLAY;
  }

  const std::string &first = completions[0];
  std::string preview = first.substr(partial.size());
  if (!preview.empty()) {
    // Insert the preview into the buffer and move cursor back so the
    // preview sits after the cursor (visual-only until accepted).
    el_insertstr(el, preview.c_str());
    // Move cursor left by preview length
    el_cursor(el, -static_cast<int>(preview.size()));
    cli->preview_len_ = static_cast<int>(preview.size());
  }

  return CC_REDISPLAY;
}

unsigned char CLI::handleBackspacePreview(EditLine *el, int ch) {
  (void)ch;
  CLI *cli = nullptr;
  el_get(el, EL_CLIENTDATA, &cli);
  if (!cli)
    return CC_ERROR;

  // Remove any active preview first
  if (cli->preview_len_ > 0) {
    el_cursor(el, cli->preview_len_);
    el_deletestr(el, cli->preview_len_);
    cli->preview_len_ = 0;
  }

  // Perform a backspace (delete one character before the cursor)
  el_deletestr(el, 1);

  // Recompute preview for the new partial word
  const LineInfo *li = el_line(el);
  if (!li)
    return CC_REDISPLAY;

  const char *cursor = li->cursor;
  const char *start = li->buffer;
  const char *word_start = cursor;
  while (word_start > start && word_start[-1] != ' ')
    word_start--;

  if (word_start >= cursor)
    return CC_REDISPLAY;

  std::string partial(word_start, cursor - word_start);
  if (partial.empty())
    return CC_REDISPLAY;

  std::string lineBeforeWord(start, word_start);
  auto tokens = cli->parser_.tokenize(lineBeforeWord);
  auto completions = cli->getCompletions(tokens, partial);
  if (completions.empty() || completions[0].size() <= partial.size())
    return CC_REDISPLAY;

  const std::string &first = completions[0];
  std::string preview = first.substr(partial.size());
  if (!preview.empty()) {
    el_insertstr(el, preview.c_str());
    el_cursor(el, -static_cast<int>(preview.size()));
    cli->preview_len_ = static_cast<int>(preview.size());
  }

  return CC_REDISPLAY;
}

unsigned char CLI::handleSpace(EditLine *el, int ch) {
  (void)ch;
  CLI *cli = nullptr;
  el_get(el, EL_CLIENTDATA, &cli);
  if (!cli)
    return CC_ERROR;

  // Remove any active preview first
  if (cli->preview_len_ > 0) {
    el_cursor(el, cli->preview_len_);
    el_deletestr(el, cli->preview_len_);
    cli->preview_len_ = 0;
  }

  // Insert the space character
  el_insertstr(el, " ");
  return CC_REDISPLAY;
}

unsigned char CLI::handleCtrlC(EditLine *el, int ch) {
  (void)ch;
  CLI *cli = nullptr;
  el_get(el, EL_CLIENTDATA, &cli);
  if (!cli)
    return CC_ERROR;

  // Clear any active preview
  if (cli->preview_len_ > 0) {
    el_cursor(el, cli->preview_len_);
    el_deletestr(el, cli->preview_len_);
    cli->preview_len_ = 0;
  }

  // Clear the entire input line
  const LineInfo *li = el_line(el);
  if (li) {
    const char *start = li->buffer;
    const char *cursor = li->cursor;
    const char *last = start ? (start + std::strlen(start)) : nullptr;
    // Move to end then delete whole buffer
    if (last && cursor) {
      el_cursor(el, static_cast<int>(last - cursor));
      el_deletestr(el, static_cast<int>(last - start));
    }
  }

  // Print newline to emulate interrupt and redisplay prompt
  std::cout << '\n';
  return CC_REDISPLAY;
}

unsigned char CLI::handleCtrlL(EditLine *el, int ch) {
  (void)ch;
  CLI *cli = nullptr;
  el_get(el, EL_CLIENTDATA, &cli);
  if (!cli)
    return CC_ERROR;

  // Clear screen using ANSI sequences and ask libedit to redraw
  std::cout << "\x1b[2J\x1b[H";
  return CC_REDISPLAY;
}

unsigned char CLI::handleCtrlD(EditLine *el, int ch) {
  (void)ch;
  CLI *cli = nullptr;
  el_get(el, EL_CLIENTDATA, &cli);
  if (!cli)
    return CC_ERROR;

  const LineInfo *li = el_line(el);
  if (!li)
    return CC_REDISPLAY;

  const char *start = li->buffer;
  const char *cursor = li->cursor;
  const char *last = start ? (start + std::strlen(start)) : nullptr;

  // If prompt is empty, exit
  if (start == cursor && last == cursor) {
    std::exit(0);
  }

  // Otherwise, delete character under cursor if any
  if (cursor < last) {
    // Move cursor right one and delete the char we moved past
    el_cursor(el, 1);
    el_deletestr(el, 1);
  }
  return CC_REDISPLAY;
}

unsigned char CLI::handleCtrlR(EditLine *el, int ch) {
  (void)ch;
  CLI *cli = nullptr;
  el_get(el, EL_CLIENTDATA, &cli);
  if (!cli)
    return CC_ERROR;

  // Determine current partial word at cursor
  const LineInfo *li = el_line(el);
  std::string partial;
  if (li) {
    const char *cursor = li->cursor;
    const char *start = li->buffer;
    const char *word_start = cursor;
    while (word_start > start && word_start[-1] != ' ')
      word_start--;
    if (word_start < cursor)
      partial.assign(word_start, cursor - word_start);
  }

  // Read history file (if available) and print recent matches containing
  // the partial string. If no history file, attempt to indicate no results.
  std::vector<std::string> lines;
  if (!cli->historyFile_.empty()) {
    std::ifstream hf(cli->historyFile_);
    std::string l;
    while (std::getline(hf, l)) {
      lines.push_back(l);
    }
  }

  if (lines.empty()) {
    std::cout << "(no history)\n";
    return CC_REDISPLAY;
  }

  // Search backwards for matches
  int printed = 0;
  for (auto it = lines.rbegin(); it != lines.rend() && printed < 10; ++it) {
    if (partial.empty() || it->find(partial) != std::string::npos) {
      std::cout << *it << "\n";
      ++printed;
    }
  }
  if (printed == 0)
    std::cout << "(no matches)\n";

  return CC_REDISPLAY;
}

// rpromptFunc removed; inline preview is inserted into the edit buffer.

void CLI::run() {
  // Check if in interactive mode (tty)
  if (!isatty(STDIN_FILENO)) {
    // Non-interactive: read from stdin line by line
    std::string line;
    while (std::getline(std::cin, line)) {
      if (line.empty() || line[0] == '#')
        continue;
      processLine(line);
    }
    return;
  }

  // Interactive mode with editline
  setupEditLine();
  if (!el_) {
    std::cerr << "Failed to initialize interactive mode\n";
    return;
  }

  int count;
  const char *line;
  for (;;) {
    line = el_gets(el_, &count);
    // If SIGINT was received, handle it first: print newline, reset editline
    // and continue. This avoids el_gets returning NULL repeatedly and
    // producing repeated prompts.
    if (g_sigint_received) {
      g_sigint_received = 0;
      if (preview_len_ > 0) {
        preview_len_ = 0;
      }
      std::cout << '\n' << std::flush;
      el_reset(el_);
      continue;
    }

    if (line == nullptr) {
      // Interrupted by a signal (e.g. Ctrl-C) or other condition. If
      // non-interactive, exit the loop; otherwise just continue.
      if (!isatty(STDIN_FILENO))
        break;
      continue;
    }

    if (count <= 0)
      continue;

    std::string cmd(line);
    // Remove trailing newline
    if (!cmd.empty() && cmd.back() == '\n')
      cmd.pop_back();

    // If an inline preview was present in the buffer, strip it from the
    // returned line before processing.
    if (preview_len_ > 0 && cmd.size() >= static_cast<size_t>(preview_len_)) {
      cmd.erase(cmd.size() - static_cast<size_t>(preview_len_));
      preview_len_ = 0;
    }

    if (cmd.empty())
      continue;

    if (cmd == "exit" || cmd == "quit")
      break;

    saveHistory(cmd);
    processLine(cmd);
  }
}
