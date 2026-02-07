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
 * @file TableFormatter.hpp
 * @brief Abstract base for table formatters with table building functionality
 */

#pragma once

#include <string>
#include <vector>

template <typename T> class TableFormatter {
public:
  struct Column {
    std::string key;   // logical key
    std::string title; // header title
    int priority = 1;  // higher = resist shrinking
    int minWidth = 3;  // minimum allowed width when shrinking
    bool leftAlign = true;
  };

  TableFormatter() = default;
  virtual ~TableFormatter() = default;

  // Format a list of data as an ASCII table (implemented by derived classes)
  virtual std::string format(const std::vector<T> &data) const = 0;

protected:
  // Table building methods for use by derived classes
  void addColumn(const std::string &key, const std::string &title,
                 int priority = 1, int minWidth = 3,
                 bool leftAlign = true) const;

  void addRow(const std::vector<std::string> &cells) const;

  void setSortColumn(int index) const;

  // Render accumulated rows/columns as formatted table string
  std::string renderTable(int maxWidth = 80) const;

  // Clear accumulated rows and columns
  void clearTable() const;

private:
  mutable std::vector<Column> columns_;
  mutable std::vector<std::vector<std::string>> rows_;
  mutable int sortColumn_ = 0;
};

// Template implementation
#include <algorithm>
#include <cctype>
#include <numeric>
#include <sstream>

namespace {
  static inline std::vector<std::string> splitLines(const std::string &s) {
    std::vector<std::string> out;
    std::string line;
    std::istringstream iss(s);
    while (std::getline(iss, line))
      out.push_back(line);
    if (out.empty())
      out.push_back("");
    return out;
  }

  static inline int visibleLength(const std::string &s) {
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

  static inline std::string truncateVisible(const std::string &s, int w) {
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
} // namespace

template <typename T>
void TableFormatter<T>::addColumn(const std::string &key,
                                  const std::string &title, int priority,
                                  int minWidth, bool leftAlign) const {
  Column c;
  c.key = key;
  c.title = title;
  c.priority = priority;
  c.minWidth = std::max(1, minWidth);
  c.leftAlign = leftAlign;
  columns_.push_back(std::move(c));
}

template <typename T>
void TableFormatter<T>::addRow(const std::vector<std::string> &cells) const {
  if (cells.size() != columns_.size())
    return;
  rows_.push_back(cells);
}

template <typename T> void TableFormatter<T>::setSortColumn(int index) const {
  sortColumn_ = index;
}

template <typename T> void TableFormatter<T>::clearTable() const {
  columns_.clear();
  rows_.clear();
  sortColumn_ = 0;
}

template <typename T>
std::string TableFormatter<T>::renderTable(int maxWidth) const {
  if (columns_.empty())
    return std::string();

  const size_t ncol = columns_.size();

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

  auto totalWidth = std::accumulate(widths.begin(), widths.end(), 0) +
                    static_cast<int>(std::max<size_t>(0, ncol - 1));

  if (totalWidth > maxWidth) {
    std::vector<size_t> idx(ncol);
    for (size_t i = 0; i < ncol; ++i)
      idx[i] = i;
    std::sort(idx.begin(), idx.end(), [&](size_t a, size_t b) {
      if (columns_[a].priority != columns_[b].priority)
        return columns_[a].priority < columns_[b].priority;
      return a < b;
    });

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

  for (size_t i = 0; i < ncol; ++i) {
    if (i)
      oss << ' ';
    oss << pad(columns_[i].title, widths[i], columns_[i].leftAlign);
  }
  oss << '\n';

  for (size_t i = 0; i < ncol; ++i) {
    if (i)
      oss << ' ';
    oss << std::string(widths[i], '-');
  }
  oss << '\n';

  std::vector<std::vector<std::string>> sorted_rows = rows_;
  int sc = sortColumn_;
  if (sc < 0 || sc >= static_cast<int>(ncol))
    sc = 0;

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
            return true;
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
