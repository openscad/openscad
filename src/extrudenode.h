#pragma once

#include "node.h"
#include "value.h"
#include "linalg.h"
#include <vector>

class ExtrudeNode : public AbstractPolyNode
{
public:
	VISITABLE();
	ExtrudeNode(const ModuleInstantiation *mi) : AbstractPolyNode(mi) {
		convexity = 0;
		is_ring = false;
	}
	virtual std::string toString() const;
	virtual std::string name() const { return "extrude"; }

	std::vector<Transform3d> path;
	int convexity;
	bool flip_faces;
	bool is_ring;
};
