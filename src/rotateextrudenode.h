#pragma once

#include "node.h"
#include "visitor.h"
#include "value.h"

class RotateExtrudeNode : public AbstractPolyNode
{
public:
	RotateExtrudeNode(const ModuleInstantiation *mi) : AbstractPolyNode(mi) {
		convexity = 0;
		fn = fs = fa = 0;
		origin_x = origin_y = scale = 0;
	}
  virtual Response accept(class State &state, Visitor &visitor) const {
		return visitor.visit(state, *this);
	}
	virtual std::string toString() const;
	virtual std::string name() const { return "rotate_extrude"; }

	int convexity;
	double fn, fs, fa;
	double origin_x, origin_y, scale;
	Filename filename;
	std::string layername;
};
