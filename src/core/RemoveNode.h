#pragma once

#include "core/ModuleInstantiation.h"
#include "core/node.h"

class RemoveNode : public AbstractNode
{
public:
  VISITABLE();
  RemoveNode(const ModuleInstantiation *mi) : AbstractNode(mi) {}
  std::string name() const override;
  std::string toString() const override;
};
