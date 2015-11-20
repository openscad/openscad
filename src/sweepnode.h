//
//  sweepnode.h
//
//
//  Created by Oskar Linde on 2014-05-17.
//
//

#pragma once

#include "node.h"
#include "visitor.h"
#include "value.h"
#include <Eigen/Geometry>

class SweepNode : public AbstractPolyNode
{
public:
	SweepNode(const ModuleInstantiation *mi) : AbstractPolyNode(mi) {
		convexity = 0;
		dimensions = 0;
	}
	virtual Response accept(class State &state, Visitor &visitor) const {
		return visitor.visit(state, *this);
	}
	virtual std::string toString() const;
	virtual std::string name() const { return "sweep"; }

	int convexity;
	int dimensions;
	std::vector<Eigen::Projective3d> sweep_path;
};
