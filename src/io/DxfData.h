#pragma once

#include <memory>
#include <string>
#include <vector>
#include "geometry/linalg.h"

class DxfData
{
public:
  struct Path {
    std::vector<int> indices; // indices into DxfData::points
    bool is_closed{false}, is_inner{false};
    Path() = default;
  };
  struct Dim {
    unsigned int type;
    double coords[7][2];
    double angle;
    double length;
    std::string name;
    Dim() {
      for (auto& coord : coords) {
        for (double& j : coord) {
          j = 0;
        }
      }
      type = 0;
      angle = 0;
      length = 0;
    }
  };

  VectorOfVector2d points;
  std::vector<Path> paths;
  std::vector<Dim> dims;

  DxfData() = default;
  DxfData(double fn, double fs, double fa,
          const std::string& filename, const std::string& layername = "",
          double xorigin = 0.0, double yorigin = 0.0, double scale = 1.0);

  int addPoint(double x, double y);

  void fixup_path_direction();
  [[nodiscard]] std::string dump() const;
  [[nodiscard]] std::unique_ptr<class Polygon2d> toPolygon2d() const;
};
