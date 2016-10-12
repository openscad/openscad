#pragma once

#include "NodeVisitor.h"
#include "enums.h"
#include "memory.h"
#include "Geometry.h"

#include <utility>
#include <list>
#include <vector>
#include <map>

class GeometryEvaluator : public NodeVisitor
{
public:
	GeometryEvaluator(const class Tree &tree);
	virtual ~GeometryEvaluator() {}

	shared_ptr<const Geometry> evaluateGeometry(const AbstractNode &node, bool allownef);

	virtual Response visit(State &state, const AbstractNode &node);
	virtual Response visit(State &state, const AbstractIntersectionNode &node);
	virtual Response visit(State &state, const AbstractPolyNode &node);
	virtual Response visit(State &state, const LinearExtrudeNode &node);
	virtual Response visit(State &state, const RotateExtrudeNode &node);
	virtual Response visit(State &state, const GroupNode &node);
	virtual Response visit(State &state, const RootNode &node);
	virtual Response visit(State &state, const LeafNode &node);
	virtual Response visit(State &state, const TransformNode &node);
	virtual Response visit(State &state, const CsgOpNode &node);
	virtual Response visit(State &state, const CgaladvNode &node);
	virtual Response visit(State &state, const ProjectionNode &node);
	virtual Response visit(State &state, const RenderNode &node);
	virtual Response visit(State &state, const TextNode &node);
	virtual Response visit(State &state, const OffsetNode &node);

	const Tree &getTree() const { return this->tree; }

private:
	class ResultObject {
	public:
		ResultObject() : is_const(true) {}
		ResultObject(const Geometry *g) : is_const(true), const_pointer(g) {}
		ResultObject(shared_ptr<const Geometry> &g) : is_const(true), const_pointer(g) {}
		ResultObject(Geometry *g) : is_const(false), pointer(g) {}
		ResultObject(shared_ptr<Geometry> &g) : is_const(false), pointer(g) {}
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

	void smartCacheInsert(const AbstractNode &node, const shared_ptr<const Geometry> &geom);
	shared_ptr<const Geometry> smartCacheGet(const AbstractNode &node, bool preferNef);
	bool isSmartCached(const AbstractNode &node);
	std::vector<const class Polygon2d *> collectChildren2D(const AbstractNode &node);
	Geometry::Geometries collectChildren3D(const AbstractNode &node);
	Polygon2d *applyMinkowski2D(const AbstractNode &node);
	Polygon2d *applyHull2D(const AbstractNode &node);
	Geometry *applyHull3D(const AbstractNode &node);
	void applyResize3D(class CGAL_Nef_polyhedron &N, const Vector3d &newsize, const Eigen::Matrix<bool,3,1> &autosize);
	Polygon2d *applyToChildren2D(const AbstractNode &node, OpenSCADOperator op);
	ResultObject applyToChildren3D(const AbstractNode &node, OpenSCADOperator op);
	ResultObject applyToChildren(const AbstractNode &node, OpenSCADOperator op);
	void addToParent(const State &state, const AbstractNode &node, const shared_ptr<const Geometry> &geom);

	std::map<int, Geometry::Geometries> visitedchildren;
	const Tree &tree;
	shared_ptr<const Geometry> root;

public:
};
