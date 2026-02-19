#include "core/Selection.h"

#include <Eigen/Dense>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>

std::string SelectionTypeToString(SelectionType type)
{
  switch (type) {
  case SelectionType::SELECTION_POINT: return "point";
  case SelectionType::SELECTION_LINE:  return "line";
  default:                             return "unknown_SelectionType";
  }
}

// FIXME: should be somewhere reusable
std::string Vector3dtoString(const Eigen::Vector3d& vec,
                             int precision = std::numeric_limits<double>::max_digits10)
{
  std::ostringstream oss;
  oss << std::setprecision(precision);

  oss << "[" << vec.x() << ", " << vec.y() << ", " << vec.z() << "]";

  return oss.str();
}

std::string SelectedObject::toString() const
{
  if (type == SelectionType::SELECTION_LINE) {
    return Vector3dtoString(p1) + " to " + Vector3dtoString(p2);
  }
  return Vector3dtoString(p1);
}
