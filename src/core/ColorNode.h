#pragma once

#include <string>

#include "core/BaseVisitable.h"
#include "core/ModuleInstantiation.h"
#include "core/node.h"
#include "geometry/linalg.h"

class ColorNode : public AbstractNode
{
public:
  VISITABLE();
  ColorNode(std::shared_ptr<const ModuleInstantiation> mi) : AbstractNode(std::move(mi)) {}
  std::string toString() const override;
  std::string name() const override;

  Color4f color;
};
