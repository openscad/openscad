#ifndef OFFSETNODE_H_
#define OFFSETNODE_H_

#include "node.h"
#include "visitor.h"
#include "value.h"
#include "clipper-utils.h"

class OffsetNode : public AbstractPolyNode
{
public:
	OffsetNode(const ModuleInstantiation *mi) : AbstractPolyNode(mi), fn(0), fs(0), fa(0), delta(1), miter_limit(2.0), join_type(ClipperLib::jtMiter) { }
        virtual Response accept(class State &state, Visitor &visitor) const {
		return visitor.visit(state, *this);
	}
	virtual std::string toString() const;
	virtual std::string name() const { return "offset"; }

	double fn, fs, fa, delta, miter_limit;
        ClipperLib::JoinType join_type;
};

#endif
