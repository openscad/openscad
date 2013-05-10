#ifndef LINEAREXTRUDENODE_H_
#define LINEAREXTRUDENODE_H_

#include "node.h"
#include "visitor.h"
#include "value.h"

class LinearExtrudeNode : public AbstractPolyNode
{
public:
	LinearExtrudeNode(const ModuleInstantiation *mi) : AbstractPolyNode(mi) {
		convexity = slices = 0;
		fn = fs = fa = height = twist = 0;
		origin_x = origin_y = 0;
		scale_x = scale_y = 1;
		center = has_twist = false;
	}
  virtual Response accept(class State &state, Visitor &visitor) const {
		return visitor.visit(state, *this);
	}
	virtual std::string toString() const;
	virtual std::string name() const { return "linear_extrude"; }

	int convexity, slices;
	double fn, fs, fa, height, twist;
	double origin_x, origin_y, scale_x, scale_y;
	bool center, has_twist;
	Filename filename;
	std::string layername;
	virtual PolySet *evaluate_polyset(class PolySetEvaluator *) const;
};

#endif
