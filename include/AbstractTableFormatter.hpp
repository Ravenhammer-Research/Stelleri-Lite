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
