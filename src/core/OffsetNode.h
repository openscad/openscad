#pragma once

#include "node.h"
#include "Value.h"
#include "ClipperUtils.h"

class OffsetNode : public AbstractPolyNode
{
public:
  VISITABLE();
  OffsetNode(const ModuleInstantiation *mi) : AbstractPolyNode(mi), chamfer(false), fn(0), fs(0), fa(0), delta(1), miter_limit(1000000.0), join_type(ClipperLib::jtRound) { }
  void print(scad::ostringstream& stream) const override final;
  std::string name() const override final { return "offset"; }

  bool chamfer;
  double fn, fs, fa, delta;
  double miter_limit; // currently fixed high value to disable chamfers with jtMiter
  ClipperLib::JoinType join_type;
};
