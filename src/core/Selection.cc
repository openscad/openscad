#include "core/Selection.h"

std::string SelectionTypeToString(SelectionType type)
{
  switch (type) {
  case SelectionType::SELECTION_POINT: return "point";
  case SelectionType::SELECTION_LINE:  return "line";
  default:                             return "unknown_SelectionType";
  }
}
