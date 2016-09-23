#pragma once

#include "node.h"
#include "visitor.h"
#include "value.h"

class BendNode: public AbstractPolyNode
{
public:
	BendNode(const ModuleInstantiation *mi): AbstractPolyNode(mi) {
		convexity = 0;
		center_x = center_y = center_z = 0;
		fixed_x = fixed_y = fixed_z = 0;
		is3d = false;
	}
	virtual Response accept(class State &state, Visitor &visitor) const {
		return visitor.visit(state, *this);
	}
	virtual std::string toString() const;
	virtual std::string name() const { return "bend"; }

	int convexity;
	bool is3d;
	// center is the center of bend circle/sphere
	double center_x, center_y, center_z;
	// fixed point, i.e. point that doesn't move after transformation
	double fixed_x, fixed_y, fixed_z;
	// this point specifies the orientation of bend cylinder.
	// bend will be spherical if all 3 points (center, fixed and cyl)
	// are laying on a single straight line
	double cyl_x, cyl_y, cyl_z;
};
