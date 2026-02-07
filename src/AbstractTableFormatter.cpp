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

#include "AbstractTableFormatter.hpp"

#include <algorithm>
#include <cctype>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

static std::vector<std::string> splitLines(const std::string &s) {
  std::vector<std::string> out;
  std::string line;
  std::istringstream iss(s);
  while (std::getline(iss, line))
    out.push_back(line);
  if (out.empty())
    out.push_back("");
  return out;
}

// Compute visible length of a string while ignoring ANSI escape sequences
static int visibleLength(const std::string &s) {
  int len = 0;
  for (size_t i = 0; i < s.size();) {
    if (s[i] == '\x1b' && i + 1 < s.size() && s[i + 1] == '[') {
      // consume until 'm' or end
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

// Truncate string to visible width `w`, preserving ANSI sequences.
static std::string truncateVisible(const std::string &s, int w) {
  std::string out;
  int vis = 0;
  for (size_t i = 0; i < s.size() && vis < w;) {
    if (s[i] == '\x1b' && i + 1 < s.size() && s[i + 1] == '[') {
      // copy entire escape
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

void AbstractTableFormatter::addColumn(const std::string &key,
                                       const std::string &title, int priority,
                                       int minWidth, bool leftAlign) {
  Column c;
  c.key = key;
  c.title = title;
  c.priority = priority;
  c.minWidth = std::max(1, minWidth);
  c.leftAlign = leftAlign;
  columns_.push_back(std::move(c));
}

void AbstractTableFormatter::addRow(const std::vector<std::string> &cells) {
  if (cells.size() != columns_.size())
    return;
  rows_.push_back(cells);
}

void AbstractTableFormatter::setSortColumn(int index) { sortColumn_ = index; }

std::string AbstractTableFormatter::format(int maxWidth) const {
  if (columns_.empty())
    return std::string();

  const size_t ncol = columns_.size();

  // Measure maximum content width per column (consider multiline cells)
  std::vector<int> widths(ncol, 0);
  for (size_t i = 0; i < ncol; ++i) {
    widths[i] = static_cast<int>(columns_[i].title.size());
  }

  for (const auto &r : rows_) {
    for (size_t i = 0; i < ncol; ++i) {
      auto lines = splitLines(r[i]);
      for (const auto &l : lines)
        widths[i] = std::max(widths[i], static_cast<int>(l.size()));
    }
  }

  // account for single-space separators between columns
  auto totalWidth = std::accumulate(widths.begin(), widths.end(), 0) +
                    static_cast<int>(std::max<size_t>(0, ncol - 1));

  // If total exceeds maxWidth, shrink lower-priority columns first
  if (totalWidth > maxWidth) {
    // Build index order by increasing priority (low priority shrinks first)
    std::vector<size_t> idx(ncol);
    for (size_t i = 0; i < ncol; ++i)
      idx[i] = i;
    std::sort(idx.begin(), idx.end(), [&](size_t a, size_t b) {
      if (columns_[a].priority != columns_[b].priority)
        return columns_[a].priority < columns_[b].priority;
      return a < b;
    });

    // Iteratively reduce columns down to minWidth until total fits or no more
    // reductions possible.
    while (totalWidth > maxWidth) {
      bool reduced = false;
      for (size_t k = 0; k < idx.size() && totalWidth > maxWidth; ++k) {
        size_t i = idx[k];
        if (widths[i] > columns_[i].minWidth) {
          widths[i] -= 1;
          totalWidth -= 1;
          reduced = true;
        }
      }
      if (!reduced)
        break;
    }
  }

  // Renderer: header, separator, rows, multiline handling
  std::ostringstream oss;

  auto pad = [&](const std::string &s, int w, bool left) {
    int vis = visibleLength(s);
    if (vis >= w)
      return truncateVisible(s, w);
    int padlen = w - vis;
    if (left) {
      std::string out = s;
      out.append(padlen, ' ');
      return out;
    } else {
      std::string out(padlen, ' ');
      out += s;
      return out;
    }
  };

  // Header
  for (size_t i = 0; i < ncol; ++i) {
    if (i)
      oss << ' ';
    oss << pad(columns_[i].title, widths[i], columns_[i].leftAlign);
  }
  oss << '\n';

  // Separator
  for (size_t i = 0; i < ncol; ++i) {
    if (i)
      oss << ' ';
    oss << std::string(widths[i], '-');
  }
  oss << '\n';

  // Rows: sort by configured column (default column 0) before rendering
  std::vector<std::vector<std::string>> sorted_rows = rows_;
  int sc = sortColumn_;
  if (sc < 0 || sc >= static_cast<int>(ncol))
    sc = 0;

  // If the column key is "Index", perform numeric sort (missing values
  // like '-' sort last). Otherwise fall back to lexicographic (stable).
  bool isIndexColumn = (sc >= 0 && sc < static_cast<int>(columns_.size()) &&
                        columns_[sc].key == "Index");

  if (isIndexColumn) {
    std::stable_sort(
        sorted_rows.begin(), sorted_rows.end(),
        [&](const std::vector<std::string> &a,
            const std::vector<std::string> &b) {
          const std::string &sa =
              (sc < static_cast<int>(a.size())) ? a[sc] : std::string();
          const std::string &sb =
              (sc < static_cast<int>(b.size())) ? b[sc] : std::string();
          auto parseInt = [&](const std::string &s, long long &out) -> bool {
            if (s.empty() || s == "-")
              return false;
            try {
              size_t pos = 0;
              out = std::stoll(s, &pos);
              return pos == s.size();
            } catch (...) {
              return false;
            }
          };
          long long ia = 0, ib = 0;
          bool ha = parseInt(sa, ia);
          bool hb = parseInt(sb, ib);
          if (ha && hb)
            return ia < ib;
          if (ha && !hb)
            return true; // numbers come before missing/non-numeric
          if (!ha && hb)
            return false;
          return sa < sb;
        });
  } else {
    std::stable_sort(sorted_rows.begin(), sorted_rows.end(),
                     [&](const std::vector<std::string> &a,
                         const std::vector<std::string> &b) {
                       const std::string &sa = (sc < static_cast<int>(a.size()))
                                                   ? a[sc]
                                                   : std::string();
                       const std::string &sb = (sc < static_cast<int>(b.size()))
                                                   ? b[sc]
                                                   : std::string();
                       return sa < sb;
                     });
  }

  for (const auto &r : sorted_rows) {
    // split each cell into lines
    std::vector<std::vector<std::string>> cellLines(ncol);
    size_t maxLines = 1;
    for (size_t i = 0; i < ncol; ++i) {
      cellLines[i] = splitLines(r[i]);
      maxLines = std::max(maxLines, cellLines[i].size());
    }

    for (size_t ln = 0; ln < maxLines; ++ln) {
      for (size_t i = 0; i < ncol; ++i) {
        if (i)
          oss << ' ';
        std::string cell = (ln < cellLines[i].size()) ? cellLines[i][ln] : "";
        oss << pad(cell, widths[i], columns_[i].leftAlign);
      }
      oss << '\n';
    }
  }

  return oss.str();
}
