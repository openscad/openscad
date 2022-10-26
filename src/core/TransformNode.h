#pragma once

#include "node.h"
#include "linalg.h"

class TransformNode : public AbstractNode
{
public:
  VISITABLE();
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW
  TransformNode(const ModuleInstantiation *mi, const std::string& verbose_name);
  void print(scad::ostringstream& stream) const override final;
  std::string name() const override final;
  std::string verbose_name() const override final;
  Transform3d matrix;

private:
  const std::string _name;
};
