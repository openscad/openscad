#pragma once

#include <memory>
#include <string>

#include "core/CurveDiscretizer.h"
#include "core/ModuleInstantiation.h"
#include "core/node.h"
#include "clipper2/clipper.h"

class OffsetNode : public AbstractPolyNode
{
public:
  VISITABLE();
  OffsetNode(const ModuleInstantiation *mi, CurveDiscretizer discretizer)
    : AbstractPolyNode(mi), discretizer(std::move(discretizer))
  {
  }

  std::string toString() const override;
  std::string name() const override { return "offset"; }

  bool chamfer{false};
  CurveDiscretizer discretizer;
  double delta{1};
  double miter_limit{1000000.0};  // currently fixed high value to disable chamfers with jtMiter
  Clipper2Lib::JoinType join_type{Clipper2Lib::JoinType::Round};
};
