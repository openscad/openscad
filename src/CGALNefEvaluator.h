#pragma once

#include "GeometryEvaluator.h"
#include "CGAL_Nef_Polyhedron.h"

class CGALNefEvaluator : public GeometryEvaluator
{
public:
	GeometryEvaluator(const class Tree &tree);
	~GeometryEvaluator() {}

	shared_ptr<const Geometry> evaluateGeometry(const AbstractNode &node, bool allownef);

	Response visit(State &state, const AbstractNode &node) override;
	Response visit(State &state, const AbstractIntersectionNode &node) override;
	Response visit(State &state, const AbstractPolyNode &node) override;
	Response visit(State &state, const LinearExtrudeNode &node) override;
	Response visit(State &state, const RotateExtrudeNode &node) override;
	Response visit(State &state, const GroupNode &node) override;
	Response visit(State &state, const RootNode &node) override;
	Response visit(State &state, const LeafNode &node) override;
	Response visit(State &state, const TransformNode &node) override;
	Response visit(State &state, const CsgOpNode &node) override;
	Response visit(State &state, const CgaladvNode &node) override;
	Response visit(State &state, const ProjectionNode &node) override;
	Response visit(State &state, const RenderNode &node) override;
	Response visit(State &state, const TextNode &node) override;
	Response visit(State &state, const OffsetNode &node) override;

	const Tree &getTree() const { return this->tree; }

private:
	class NefResult {
	public:
		NefResult() : is_const(true) {}
		NefResult(const Geometry *g) : is_const(true), const_pointer(g) {}
		NefResult(shared_ptr<const Geometry> &g) : is_const(true), const_pointer(g) {}
		NefResult(Geometry *g) : is_const(false), pointer(g) {}
		NefResult(shared_ptr<Geometry> &g) : is_const(false), pointer(g) {}
		bool isConst() const { return is_const; }
		shared_ptr<Geometry> ptr() { assert(!is_const); return pointer; }
		shared_ptr<const Geometry> constptr() const { 
			return is_const ? const_pointer : static_pointer_cast<const Geometry>(pointer);
		}
	private:
		bool is_const;
		shared_ptr<Geometry> pointer;
		shared_ptr<const Geometry> const_pointer;
	};

	Geometry::Geometries collectChildren3D(const AbstractNode &node);
	Polygon2d *applyMinkowski2D(const AbstractNode &node);
	Polygon2d *applyHull2D(const AbstractNode &node);
	Geometry *applyHull3D(const AbstractNode &node);
	void applyResize3D(Geometry &N, const Vector3d &newsize, const Eigen::Matrix<bool,3,1> &autosize);
	Polygon2d *applyToChildren2D(const AbstractNode &node, OpenSCADOperator op);
	NefResult applyToChildren3D(const AbstractNode &node, OpenSCADOperator op);
	NefResult applyToChildren(const AbstractNode &node, OpenSCADOperator op);
	void addToParent(const State &state, const AbstractNode &node, const shared_ptr<const Geometry> &geom);

	std::map<int, Geometry::Geometries> visitedchildren;
	const Tree &tree;
	shared_ptr<const Geometry> root;

public:
};
