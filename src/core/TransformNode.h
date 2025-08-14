#pragma once

#include <string>

#include "core/node.h"
#include "core/ModuleInstantiation.h"
#include "geometry/linalg.h"
#include "geometry/PolySet.h"

class TransformNode : public AbstractNode
{
public:
  VISITABLE();
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW
  TransformNode(const ModuleInstantiation *mi, std::string verbose_name);
  std::string toString() const override;
  std::string name() const override;
  std::string verbose_name() const override;
  Transform3d matrix;
  virtual std::shared_ptr<const Geometry> dragPoint(const Vector3d& pt, const Vector3d& delta,
                                                    DragResult& result) override;
  Transform3d matrix_;
  int dragflags = 0;

private:
  const std::string _name;
};
