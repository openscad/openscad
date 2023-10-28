#pragma once

#include "node.h"
#include "enums.h"

class CsgOpNode : public AbstractNode
{
public:
  VISITABLE();
  OpenSCADOperator type;
  CsgOpNode(OpenSCADOperator type) : CsgOpNode(new ModuleInstantiation("csg_op_node"), type) { }
  CsgOpNode(ModuleInstantiation *mi, OpenSCADOperator type) : AbstractNode(mi), type(type) { }
  std::string toString() const override;
  std::string name() const override;
  static std::shared_ptr<CsgOpNode> union_();
  static std::shared_ptr<CsgOpNode> intersection();
  static std::shared_ptr<CsgOpNode> difference();
};
