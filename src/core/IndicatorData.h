#pragma once

#include <string>
#include <utility>
#include <utility>

struct IndicatorData
{
  IndicatorData(int firstLine, int firstCol, int lastLine, int lastCol,
                std::string path)
    : first_line(firstLine), first_col(firstCol), last_line(lastLine),
    last_col(lastCol), path(std::move(path)) {
  }

  int first_line;
  int first_col;
  int last_line;
  int last_col;
  std::string path;
};
