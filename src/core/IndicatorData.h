#pragma once

#include <utility>
#include <filesystem>

namespace fs = std::filesystem;

struct IndicatorData {
  IndicatorData(int firstLine, int firstCol, int lastLine, int lastCol, fs::path path)
    : first_line(firstLine),
      first_col(firstCol),
      last_line(lastLine),
      last_col(lastCol),
      path(std::move(path))
  {
  }

  int first_line;
  int first_col;
  int last_line;
  int last_col;
  fs::path path;
};
