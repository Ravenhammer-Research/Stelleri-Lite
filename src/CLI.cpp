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
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sys/stat.h>
#include <unistd.h>

#include <signal.h>

// ── SIGINT handling ──────────────────────────────────────────────────────
// Install our own handler that sets a flag so the REPL can reset editline
// state without exiting.  The previous action is saved and restored on
// cleanup.
static struct sigaction g_old_sigint_action;
static volatile sig_atomic_t g_sigint_received = 0;

static void cli_sigint_handler(int) { g_sigint_received = 1; }

// =====================================================================
// Construction / destruction / history
// =====================================================================

CLI::CLI(std::unique_ptr<ConfigurationManager> mgr)
    : mgr_(std::move(mgr)), el_(nullptr), hist_(nullptr) {
  const char *home = getenv("HOME");
  if (home)
    historyFile_ = std::string(home) + "/.netcli_history";
}

CLI::~CLI() { cleanupEditLine(); }

void CLI::loadHistory() {
  if (!hist_ || historyFile_.empty())
    return;
  history(hist_, &ev_, H_LOAD, historyFile_.c_str());
}

void CLI::saveHistory(const std::string &line) {
  if (!hist_ || historyFile_.empty() || line.empty())
    return;
  history(hist_, &ev_, H_ENTER, line.c_str());
  history(hist_, &ev_, H_SAVE, historyFile_.c_str());
}

std::vector<std::string> CLI::loadHistoryLines() const {
  std::vector<std::string> lines;
  if (historyFile_.empty())
    return lines;
  std::ifstream hf(historyFile_);
  std::string l;
  while (std::getline(hf, l))
    lines.push_back(l);
  return lines;
}

// =====================================================================
// Command processing
// =====================================================================

void CLI::processLine(const std::string &line) {
  if (line.empty())
    return;
  if (line == "exit" || line == "quit")
    std::exit(0);

  saveHistory(line);

  netcli::Parser parser;
  auto toks = parser.tokenize(line);
  try {
    auto cmd = parser.parse(toks);
    if (!cmd || !cmd->head()) {
      std::cerr << "Error: Invalid command\n";
      return;
    }

    netcli::CommandDispatcher dispatcher;
    dispatcher.dispatch(cmd->head(), mgr_.get());
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << "\n";
  }
}

// =====================================================================
// Prompt callbacks
// =====================================================================

char *CLI::promptFunc(EditLine *el [[maybe_unused]]) {
  static char prompt[] = "net> ";
  return prompt;
}

char *CLI::searchPromptFunc(EditLine *el) {
  CLI *cli = nullptr;
  el_get(el, EL_CLIENTDATA, &cli);
  // Build the search prompt: (reverse-i-search)`query':
  // We use a static buffer that persists across calls.
  static char buf[256];
  if (cli)
    snprintf(buf, sizeof(buf),
             "(reverse-i-search)`%s': ", cli->search_query_.c_str());
  else
    snprintf(buf, sizeof(buf), "(reverse-i-search)`': ");
  return buf;
}

// =====================================================================
// editline setup / teardown
// =====================================================================

void CLI::setupEditLine() {
  el_ = el_init("net", stdin, stdout, stderr);
  if (!el_) {
    std::cerr << "Failed to initialize editline\n";
    return;
  }

  // SIGINT: set a flag instead of killing the process.
  struct sigaction sa;
  sa.sa_handler = cli_sigint_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGINT, &sa, &g_old_sigint_action);

  el_set(el_, EL_PROMPT, &CLI::promptFunc);
  el_set(el_, EL_EDITOR, "emacs");
  el_set(el_, EL_CLIENTDATA, this);

  // ── Tab completion ─────────────────────────────────────────────────
  el_set(el_, EL_ADDFN, "ed-complete", "Complete command",
         &CLI::completeCommand);
  el_set(el_, EL_BIND, "^I", "ed-complete", nullptr);

  // ── Inline preview on printable keys ───────────────────────────────
  el_set(el_, EL_ADDFN, "ed-show-preview", "Show inline preview",
         &CLI::showInlinePreview);
  const char *printables =
      "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";
  for (const char *p = printables; *p; ++p) {
    char bind[2] = {*p, '\0'};
    el_set(el_, EL_BIND, bind, "ed-show-preview", nullptr);
  }

  // ── Backspace with preview update ──────────────────────────────────
  el_set(el_, EL_ADDFN, "ed-backspace-preview", "Backspace with preview",
         &CLI::handleBackspacePreview);
  el_set(el_, EL_BIND, "^H", "ed-backspace-preview", nullptr);
  el_set(el_, EL_BIND, "^?", "ed-backspace-preview", nullptr);

  // ── Space clears preview then inserts ──────────────────────────────
  el_set(el_, EL_ADDFN, "ed-space-preview", "Space with preview clear",
         &CLI::handleSpace);
  el_set(el_, EL_BIND, "\\x20", "ed-space-preview", nullptr);

  // ── Control keys ───────────────────────────────────────────────────
  el_set(el_, EL_ADDFN, "ed-ctrlc", "Cancel line (Ctrl-C)", &CLI::handleCtrlC);
  el_set(el_, EL_BIND, "^C", "ed-ctrlc", nullptr);

  el_set(el_, EL_ADDFN, "ed-ctrll", "Clear screen (Ctrl-L)", &CLI::handleCtrlL);
  el_set(el_, EL_BIND, "^L", "ed-ctrll", nullptr);

  el_set(el_, EL_ADDFN, "ed-ctrld", "EOF/Delete (Ctrl-D)", &CLI::handleCtrlD);
  el_set(el_, EL_BIND, "^D", "ed-ctrld", nullptr);

  el_set(el_, EL_ADDFN, "ed-ctrl-r", "Reverse search history",
         &CLI::handleCtrlR);
  el_set(el_, EL_BIND, "^R", "ed-ctrl-r", nullptr);

  // ── History ────────────────────────────────────────────────────────
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
  (void)sigaction(SIGINT, &g_old_sigint_action, nullptr);
}

// =====================================================================
// Completion helpers  (instance methods — no copy-paste)
// =====================================================================

std::vector<std::string>
CLI::getCompletions(const std::vector<std::string> &tokens,
                    const std::string &partial) const {
  if (tokens.empty()) {
    std::vector<std::string> top = {"show", "set", "delete", "exit", "quit"};
    std::vector<std::string> m;
    for (const auto &c : top)
      if (c.rfind(partial, 0) == 0)
        m.push_back(c);
    return m;
  }

  auto cmd = parser_.parse(tokens);
  if (!cmd || !cmd->head())
    return {};

  // Walk to the tail token.
  auto tok = cmd->head();
  while (tok && tok->hasNext())
    tok = tok->getNext();
  if (!tok)
    return {};

  // Use the context-aware virtual — tokens that need the manager (e.g.
  // InterfaceToken) override it; all others fall through to
  // autoComplete(partial).
  return tok->autoComplete(tokens, partial, mgr_.get());
}

/// Remove the inline preview from the edit buffer.
void CLI::clearPreview() {
  if (preview_len_ <= 0)
    return;
  // The preview sits *after* the cursor.  Move to its end, delete it.
  el_cursor(el_, preview_len_);
  el_deletestr(el_, preview_len_);
  preview_len_ = 0;
}

/// Compute completions for the current line and show an inline preview
/// of the first match's remaining suffix (inserted after the cursor).
void CLI::computeAndShowPreview() {
  const LineInfo *li = el_line(el_);
  if (!li)
    return;

  const char *cursor = li->cursor;
  const char *start = li->buffer;

  // Find the start of the word being typed.
  const char *word = cursor;
  while (word > start && word[-1] != ' ')
    --word;

  if (word >= cursor)
    return; // no partial word

  std::string partial(word, cursor - word);
  if (partial.empty())
    return;

  // Tokenize everything before the current word.
  std::string before(start, word);
  auto tokens = parser_.tokenize(before);

  auto completions = getCompletions(tokens, partial);
  if (completions.empty() || completions[0].size() <= partial.size())
    return;

  std::string suffix = completions[0].substr(partial.size());
  if (suffix.empty())
    return;

  el_insertstr(el_, suffix.c_str());
  el_cursor(el_, -static_cast<int>(suffix.size()));
  preview_len_ = static_cast<int>(suffix.size());
}

/// Clear the entire edit buffer (for Ctrl-C, etc.).
void CLI::clearLineBuffer() {
  const LineInfo *li = el_line(el_);
  if (!li)
    return;
  const char *start = li->buffer;
  const char *cursor = li->cursor;
  const char *last = start ? (start + std::strlen(start)) : nullptr;
  if (last && cursor && last > start) {
    el_cursor(el_, static_cast<int>(last - cursor));
    el_deletestr(el_, static_cast<int>(last - start));
  }
}

// =====================================================================
// libedit key handlers (static — retrieve CLI* via EL_CLIENTDATA)
// =====================================================================

unsigned char CLI::completeCommand(EditLine *el, int ch [[maybe_unused]]) {
  CLI *cli = nullptr;
  el_get(el, EL_CLIENTDATA, &cli);
  if (!cli)
    return CC_ERROR;

  cli->clearPreview();

  const LineInfo *li = el_line(el);
  if (!li)
    return CC_ERROR;

  // Extract partial word.
  const char *cursor = li->cursor;
  const char *start = li->buffer;
  const char *word = cursor;
  while (word > start && word[-1] != ' ')
    --word;

  std::string partial(word, cursor - word);
  std::string before(start, word);
  auto tokens = cli->parser_.tokenize(before);
  auto completions = cli->getCompletions(tokens, partial);

  if (completions.empty()) {
    el_beep(el);
    return CC_REFRESH;
  }

  if (completions.size() == 1) {
    std::string ins = completions[0].substr(partial.size());
    if (!ins.empty())
      el_insertstr(el, ins.c_str());
    return CC_REFRESH;
  }

  // Multiple matches — display them.
  std::cout << '\n';
  for (const auto &c : completions)
    std::cout << c << "  ";
  std::cout << '\n';
  return CC_REDISPLAY;
}

unsigned char CLI::showInlinePreview(EditLine *el, int ch) {
  CLI *cli = nullptr;
  el_get(el, EL_CLIENTDATA, &cli);
  if (!cli)
    return CC_ERROR;

  cli->clearPreview();

  // Insert the typed character.
  char cbuf[2] = {static_cast<char>(ch), '\0'};
  el_insertstr(el, cbuf);

  cli->computeAndShowPreview();
  return CC_REDISPLAY;
}

unsigned char CLI::handleBackspacePreview(EditLine *el,
                                          int ch [[maybe_unused]]) {
  CLI *cli = nullptr;
  el_get(el, EL_CLIENTDATA, &cli);
  if (!cli)
    return CC_ERROR;

  cli->clearPreview();
  el_deletestr(el, 1); // delete char before cursor
  cli->computeAndShowPreview();
  return CC_REDISPLAY;
}

unsigned char CLI::handleSpace(EditLine *el, int ch [[maybe_unused]]) {
  CLI *cli = nullptr;
  el_get(el, EL_CLIENTDATA, &cli);
  if (!cli)
    return CC_ERROR;

  cli->clearPreview();
  el_insertstr(el, " ");
  return CC_REDISPLAY;
}

// ── Ctrl-C: cancel line ──────────────────────────────────────────────
unsigned char CLI::handleCtrlC(EditLine *el, int ch [[maybe_unused]]) {
  CLI *cli = nullptr;
  el_get(el, EL_CLIENTDATA, &cli);
  if (!cli)
    return CC_ERROR;

  cli->clearPreview();
  cli->clearLineBuffer();
  std::cout << '\n';
  return CC_REDISPLAY;
}

// ── Ctrl-L: clear screen ────────────────────────────────────────────
unsigned char CLI::handleCtrlL(EditLine *el, int ch [[maybe_unused]]) {
  CLI *cli = nullptr;
  el_get(el, EL_CLIENTDATA, &cli);
  if (!cli)
    return CC_ERROR;

  // ANSI: clear entire screen and move cursor to top-left.
  std::cout << "\x1b[2J\x1b[H" << std::flush;
  return CC_REDISPLAY;
}

// ── Ctrl-D: EOF on empty line, delete-char otherwise ─────────────────
unsigned char CLI::handleCtrlD(EditLine *el, int ch [[maybe_unused]]) {
  CLI *cli = nullptr;
  el_get(el, EL_CLIENTDATA, &cli);
  if (!cli)
    return CC_ERROR;

  const LineInfo *li = el_line(el);
  if (!li)
    return CC_REDISPLAY;

  const char *start = li->buffer;
  const char *cursor = li->cursor;

  // Compute real length (excluding any preview text at the end).
  size_t total = std::strlen(start);
  size_t real_len = total - static_cast<size_t>(std::max(0, cli->preview_len_));

  // Empty line → exit.
  if (real_len == 0 && cursor == start) {
    std::cout << '\n';
    std::exit(0);
  }

  // Otherwise delete character under cursor (if any, before preview).
  const char *real_end = start + real_len;
  if (cursor < real_end) {
    cli->clearPreview();
    el_cursor(el, 1);
    el_deletestr(el, 1);
    cli->computeAndShowPreview();
  }
  return CC_REDISPLAY;
}

// =====================================================================
// Ctrl-R: Interactive reverse-i-search
// =====================================================================
//
// Entering Ctrl-R switches the prompt to "(reverse-i-search)`': " and
// enters a sub-loop where:
//   • Printable keys extend the search query and update the match.
//   • Backspace shrinks the query.
//   • Ctrl-R cycles to the next older match.
//   • Enter / Right arrow accepts the current match into the main buffer.
//   • Ctrl-C / Ctrl-G / Escape cancels and restores the original line.

unsigned char CLI::handleCtrlR(EditLine *el, int ch [[maybe_unused]]) {
  CLI *cli = nullptr;
  el_get(el, EL_CLIENTDATA, &cli);
  if (!cli)
    return CC_ERROR;

  cli->clearPreview();
  cli->runReverseSearch();
  return CC_REDISPLAY;
}

void CLI::runReverseSearch() {
  // Save the current line so we can restore on cancel.
  const LineInfo *li = el_line(el_);
  std::string saved_line;
  if (li)
    saved_line.assign(li->buffer, std::strlen(li->buffer));

  // Load all history lines (most-recent last).
  auto history_lines = loadHistoryLines();

  // Initialise search state.
  search_query_.clear();
  search_matches_.clear();
  search_index_ = -1;
  in_search_ = true;

  // Switch to the search prompt.
  el_set(el_, EL_PROMPT, &CLI::searchPromptFunc);

  // Show empty match state initially.
  clearLineBuffer();
  el_set(el_, EL_REFRESH);

  // Helper: recompute matches for the current query.
  auto recomputeMatches = [&]() {
    search_matches_.clear();
    search_index_ = -1;
    if (search_query_.empty())
      return;
    // Walk history in reverse (most-recent first).
    for (auto it = history_lines.rbegin(); it != history_lines.rend(); ++it) {
      if (it->find(search_query_) != std::string::npos)
        search_matches_.push_back(*it);
    }
    if (!search_matches_.empty())
      search_index_ = 0;
  };

  // Helper: display the current match in the buffer.
  auto showMatch = [&]() {
    clearLineBuffer();
    if (search_index_ >= 0 &&
        search_index_ < static_cast<int>(search_matches_.size())) {
      el_insertstr(el_, search_matches_[search_index_].c_str());
    }
  };

  // ── sub-loop: read raw characters ──────────────────────────────────
  for (;;) {
    // Force a redisplay so the user sees the search prompt + current match.
    showMatch();
    el_set(el_, EL_REFRESH);

    // Read a single character.
    char c;
    ssize_t n = read(STDIN_FILENO, &c, 1);
    if (n <= 0)
      break; // EOF or error → cancel

    if (c == '\x12') {
      // Ctrl-R again: cycle to next match.
      if (!search_matches_.empty()) {
        search_index_ =
            (search_index_ + 1) % static_cast<int>(search_matches_.size());
      }
      continue;
    }

    if (c == '\r' || c == '\n') {
      // Enter: accept the current match.
      break;
    }

    // Ctrl-C (0x03), Ctrl-G (0x07), Escape (0x1b): cancel search.
    if (c == '\x03' || c == '\x07' || c == '\x1b') {
      // Restore original line.
      clearLineBuffer();
      if (!saved_line.empty())
        el_insertstr(el_, saved_line.c_str());
      break;
    }

    // Backspace (0x08, 0x7f): shrink query.
    if (c == '\x08' || c == '\x7f') {
      if (!search_query_.empty()) {
        search_query_.pop_back();
        recomputeMatches();
      }
      continue;
    }

    // Printable character: extend query.
    if (c >= 0x20 && c <= 0x7e) {
      search_query_ += c;
      recomputeMatches();
      continue;
    }

    // Any other control character — ignore.
  }

  // Restore normal prompt.
  in_search_ = false;
  el_set(el_, EL_PROMPT, &CLI::promptFunc);
}

// =====================================================================
// REPL
// =====================================================================

void CLI::run() {
  // Non-interactive: read from stdin line by line.
  if (!isatty(STDIN_FILENO)) {
    std::string line;
    while (std::getline(std::cin, line)) {
      if (line.empty() || line[0] == '#')
        continue;
      processLine(line);
    }
    return;
  }

  // Interactive mode with editline.
  setupEditLine();
  if (!el_) {
    std::cerr << "Failed to initialize interactive mode\n";
    return;
  }

  int count;
  const char *line;
  for (;;) {
    line = el_gets(el_, &count);

    // Handle SIGINT (Ctrl-C at the OS level, before our handler fires).
    if (g_sigint_received) {
      g_sigint_received = 0;
      preview_len_ = 0;
      std::cout << '\n' << std::flush;
      el_reset(el_);
      continue;
    }

    if (!line) {
      if (!isatty(STDIN_FILENO))
        break;
      continue;
    }
    if (count <= 0)
      continue;

    std::string cmd(line);
    if (!cmd.empty() && cmd.back() == '\n')
      cmd.pop_back();

    // Strip any trailing preview text that was in the buffer.
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
