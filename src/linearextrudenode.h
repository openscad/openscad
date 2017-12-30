#pragma once

#include "node.h"
#include "value.h"

class LinearExtrudeNode : public AbstractPolyNode
{
public:
	VISITABLE();
	LinearExtrudeNode(const ModuleInstantiation *mi) : AbstractPolyNode(mi) {
		convexity = slices = 0;
		fn = fs = fa = height = twist = 0;
		origin_x = origin_y = 0;
		scale_x = scale_y = 1;
		center = has_twist = false;
	}
	std::string toString() const override;
	std::string name() const override { return "linear_extrude"; }

	int convexity, slices;
	double fn, fs, fa, height, twist;
	double origin_x, origin_y, scale_x, scale_y;
	bool center, has_twist;
	Filename filename;
	std::string layername;
};
