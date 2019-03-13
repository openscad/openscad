#pragma once

#include "BaseGeometryEvaluator.h"

class CGALNefEvaluator : public GeometryEvaluator
{
public:
	CGALNefEvaluator(const class Tree &tree);
	~CGALNefEvaluator() = default;

	shared_ptr<const Geometry> evaluateGeometry(const AbstractNode &node, bool allownef) override;

	Response visit(State &state, const TransformNode &node) override;
	Response visit(State &state, const CgaladvNode &node) override;
	Response visit(State &state, const ProjectionNode &node) override;
	Response visit(State &state, const RenderNode &node) override;

protected:
	void smartCacheInsert(const AbstractNode &node, const shared_ptr<const Geometry> &geom) override;
	shared_ptr<const Geometry> smartCacheGet(const AbstractNode &node, bool preferNef) override;
	bool isSmartCached(const AbstractNode &node) override;
	ResultObject applyToChildren3D(const AbstractNode &node, OpenSCADOperator op) override;
	Geometry *applyHull3D(const AbstractNode &node);

};

