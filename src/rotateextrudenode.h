#pragma once

#include "node.h"
#include "value.h"

class RotateExtrudeNode : public AbstractPolyNode
{
public:
	VISITABLE();
	RotateExtrudeNode(const ModuleInstantiation *mi) : AbstractPolyNode(mi) {
		convexity = 0;
		fn = fs = fa = 0;
		origin_x = origin_y = scale = 0;
		angle = 360;
	}
	virtual std::string toString() const;
	virtual std::string name() const { return "rotate_extrude"; }

	int convexity;
	double fn, fs, fa;
	double origin_x, origin_y, scale, angle;
	Filename filename;
	std::string layername;
};
