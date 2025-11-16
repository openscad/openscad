#pragma once

#include <string>

#include "core/BaseVisitable.h"
#include "core/node.h"
#include "core/ModuleInstantiation.h"
#include "geometry/linalg.h"

class ColorNode : public AbstractNode
{
public:
  VISITABLE();
  ColorNode(const ModuleInstantiation *mi) : AbstractNode(mi) {}
  std::string toString() const override;
  std::string name() const override;

  Color4f color;
};
