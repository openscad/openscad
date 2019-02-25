#pragma once

#include "linalg.h"
#include <vector>

class DxfData
{
public:
  struct Path {
    std::vector<int> indices; // indices into DxfData::points
    bool is_closed, is_inner;
    Path() : is_closed(false), is_inner(false) { }
  };
  struct Dim {
    unsigned int type;
    double coords[7][2];
    double angle;
    double length;
    std::string name;
    Dim() {
      for (int i = 0; i < 7; i++)
        for (int j = 0; j < 2; j++)
          coords[i][j] = 0;
      type = 0;
      angle = 0;
      length = 0;
    }
  };

  std::vector<Vector2d> points;
  std::vector<Path> paths;
  std::vector<Dim> dims;

  DxfData();
  DxfData(double fn, double fs, double fa,
          const std::string &filename, const std::string &layername = "",
          double xorigin = 0.0, double yorigin = 0.0, double scale = 1.0);

  int addPoint(double x, double y);

  void fixup_path_direction();
  std::string dump() const;
  class Polygon2d *toPolygon2d() const;
};
