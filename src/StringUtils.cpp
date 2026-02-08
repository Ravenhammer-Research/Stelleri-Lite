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

#include "StringUtils.hpp"
#include <sstream>

namespace strutil {

  std::vector<std::string> splitLines(const std::string &s) {
    std::vector<std::string> out;
    std::string line;
    std::istringstream iss(s);
    while (std::getline(iss, line))
      out.push_back(line);
    if (out.empty())
      out.push_back("");
    return out;
  }

  int visibleLength(const std::string &s) {
    int len = 0;
    for (size_t i = 0; i < s.size();) {
      if (s[i] == '\x1b' && i + 1 < s.size() && s[i + 1] == '[') {
        i += 2;
        while (i < s.size() && s[i] != 'm')
          ++i;
        if (i < s.size() && s[i] == 'm')
          ++i;
      } else {
        ++len;
        ++i;
      }
    }
    return len;
  }

  std::string truncateVisible(const std::string &s, int w) {
    std::string out;
    int vis = 0;
    for (size_t i = 0; i < s.size() && vis < w;) {
      if (s[i] == '\x1b' && i + 1 < s.size() && s[i + 1] == '[') {
        size_t start = i;
        i += 2;
        while (i < s.size() && s[i] != 'm')
          ++i;
        if (i < s.size() && s[i] == 'm')
          ++i;
        out.append(s.data() + start, i - start);
      } else {
        out.push_back(s[i]);
        ++vis;
        ++i;
      }
    }
    return out;
  }

  std::string stripAnsi(const std::string &s) {
    std::string clean;
    bool inEscape = false;
    for (char c : s) {
      if (c == '\x1b') {
        inEscape = true;
      } else if (inEscape) {
        if (c == 'm' || c == 'K' || c == 'J' || c == 'H')
          inEscape = false;
      } else {
        clean += c;
      }
    }
    return clean;
  }

} // namespace strutil
