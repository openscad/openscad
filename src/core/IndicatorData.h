#pragma once

struct IndicatorData
{
  IndicatorData(int firstLine, int firstCol, int lastLine, int lastCol,
           std::string path)
    : first_line(firstLine), first_col(firstCol), last_line(lastLine),
    last_col(lastCol), path(path) {
  }

  ~IndicatorData()
  {
  }

  int first_line;
  int first_col;
  int last_line;
  int last_col;
  std::string path;
};
