#pragma once

#include <memory>
#include <string>

#include "core/ModuleInstantiation.h"
#include "core/node.h"
#include "clipper2/clipper.h"

class TessellationControl;

class OffsetNode : public AbstractPolyNode
{
public:
  VISITABLE();
  OffsetNode(const ModuleInstantiation *mi, std::shared_ptr<TessellationControl> tessFIXME)
    : AbstractPolyNode(mi), tessFIXME(std::move(tessFIXME))
  {
  }

  ~OffsetNode();

  std::string toString() const override;
  std::string name() const override { return "offset"; }

  bool chamfer{false};
  std::shared_ptr<TessellationControl> tessFIXME;
  double delta{1};
  double miter_limit{1000000.0};  // currently fixed high value to disable chamfers with jtMiter
  Clipper2Lib::JoinType join_type{Clipper2Lib::JoinType::Round};
};
