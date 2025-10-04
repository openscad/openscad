#pragma once

#include "core/ModuleInstantiation.h"
#include "core/node.h"
#include "core/TessellationControl.h"
#include "clipper2/clipper.h"

#include <string>

class OffsetNode : public AbstractPolyNode
{
public:
  VISITABLE();
  OffsetNode(const ModuleInstantiation *mi, TessellationControl&& tess)
    : AbstractPolyNode(mi), tess(tess)
  {
  }
  std::string toString() const override;
  std::string name() const override { return "offset"; }

  bool chamfer{false};
  TessellationControl tess;
  double delta{1};
  double miter_limit{1000000.0};  // currently fixed high value to disable chamfers with jtMiter
  Clipper2Lib::JoinType join_type{Clipper2Lib::JoinType::Round};
};
