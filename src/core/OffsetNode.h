#pragma once

#include "core/node.h"
#include "clipper2/clipper.h"

#include <string>

class OffsetNode : public AbstractPolyNode
{
public:
  VISITABLE();
  OffsetNode(const ModuleInstantiation *mi) : AbstractPolyNode(mi) { }
  std::string toString() const override;
  std::string name() const override { return "offset"; }

  bool chamfer{false};
  double fn{0}, fs{0}, fa{0}, delta{1};
  double miter_limit{1000000.0}; // currently fixed high value to disable chamfers with jtMiter
  Clipper2Lib::JoinType join_type{Clipper2Lib::JoinType::Round};
};
