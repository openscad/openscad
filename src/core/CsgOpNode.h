#pragma once

#include <string>

#include "core/ModuleInstantiation.h"
#include "core/enums.h"
#include "core/node.h"

class CsgOpNode : public AbstractNode
{
public:
  VISITABLE();
  OpenSCADOperator type;
  double r = 0.0;  // Radius for fillets
  int fn = 2;
  CsgOpNode(const ModuleInstantiation *mi, OpenSCADOperator type) : AbstractNode(mi), type(type) {}
  std::string toString() const override;
  std::string name() const override;
  virtual std::shared_ptr<const Geometry> dragPoint(const Vector3d& pt, const Vector3d& delta,
                                                    DragResult& result) override;
};
