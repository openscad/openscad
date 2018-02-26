#pragma once

#include "node.h"
#include "value.h"
#include "clipper-utils.h"

class OffsetNode : public AbstractPolyNode
{
public:
	VISITABLE();
	OffsetNode(const ModuleInstantiation *mi) : AbstractPolyNode(mi), fn(0), fs(0), fa(0), delta(1), miter_limit(1000000.0), join_type(ClipperLib::jtRound) { }
	std::string toString() const override;
	std::string name() const override { return "offset"; }

        bool chamfer;
	double fn, fs, fa, delta;
        double miter_limit; // currently fixed high value to disable chamfers with jtMiter
        ClipperLib::JoinType join_type;
};
