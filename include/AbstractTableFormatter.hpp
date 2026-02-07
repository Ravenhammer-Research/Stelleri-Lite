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

#pragma once

#include <optional>
#include <string>
#include <vector>

class AbstractTableFormatter {
public:
  struct Column {
    std::string key;   // logical key
    std::string title; // header title
    int priority = 1;  // higher = resist shrinking
    int minWidth = 3;  // minimum allowed width when shrinking
    bool leftAlign = true;
  };

  AbstractTableFormatter() = default;

  void addColumn(const std::string &key, const std::string &title,
                 int priority = 1, int minWidth = 3, bool leftAlign = true);

  // Add a row of cells. `cells` must have same length as columns added.
  void addRow(const std::vector<std::string> &cells);

  // Render table with optional maximum total width (including single space
  // separators). Default maxWidth=80.
  std::string format(int maxWidth = 80) const;

  // Set which column index to sort rows by before formatting. Default=0.
  void setSortColumn(int index);

private:
  std::vector<Column> columns_;
  std::vector<std::vector<std::string>> rows_;
  int sortColumn_ = 0;
};
