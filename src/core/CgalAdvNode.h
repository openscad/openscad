#pragma once

#include "node.h"
#include "Value.h"
#include "linalg.h"

enum class CgalAdvType {
  MINKOWSKI,
  HULL,
  FILL,
  RESIZE
};

class CgalAdvNode : public AbstractNode
{
public:
  VISITABLE();
  CgalAdvNode(const ModuleInstantiation *mi, CgalAdvType type) : AbstractNode(mi), type(type) {
    convexity = 1;
  }
  void print(scad::ostringstream& stream) const override;
  std::string name() const override;

  unsigned int convexity;
  Vector3d newsize;
  Eigen::Matrix<bool, 3, 1> autosize;
  CgalAdvType type;
};
