#pragma once

#include "node.h"
#include "value.h"

enum primitive_type_e {
	CUBE,
	SPHERE,
	CYLINDER,
	POLYHEDRON,
	SQUARE,
	CIRCLE,
	POLYGON
};

class PrimitiveNode : public LeafNode
{
public:
	PrimitiveNode(const ModuleInstantiation *mi, primitive_type_e type);
	VISITABLE();
	virtual std::string toString() const = 0;
	virtual std::string name() const = 0;
	virtual const Geometry *createGeometry() const = 0;

	primitive_type_e type;
	double fn, fs, fa;
	int convexity;
};

class CubeNode : public PrimitiveNode
{
public:
	VISITABLE();
	CubeNode(const ModuleInstantiation *mi, double x, double y, double z, bool center) : PrimitiveNode(mi, CUBE), x(x), y(y), z(z), center(center) { }
	virtual std::string toString() const;
	virtual std::string name() const { return "cube"; }
	virtual const Geometry *createGeometry() const;

	double x, y, z;
	bool center;
};

class SphereNode : public PrimitiveNode
{
public:
	VISITABLE();
	SphereNode(const ModuleInstantiation *mi, double r) : PrimitiveNode(mi, SPHERE), r(r) { }
	virtual std::string toString() const;
	virtual std::string name() const { return "sphere"; }
	virtual const Geometry *createGeometry() const;

	double r;
};

class CylinderNode : public PrimitiveNode
{
public:
	VISITABLE();
	CylinderNode(const ModuleInstantiation *mi, double r1, double r2, double h, bool center) : PrimitiveNode(mi, CYLINDER), r1(r1), r2(r2), h(h), center(center) { }
	virtual std::string toString() const;
	virtual std::string name() const { return "cylinder"; }
	virtual const Geometry *createGeometry() const;

	double r1, r2, h;
	bool center;
};

class PolyhedronNode : public PrimitiveNode
{
public:
	VISITABLE();
	PolyhedronNode(const ModuleInstantiation *mi, ValuePtr points, ValuePtr faces) : PrimitiveNode(mi, POLYHEDRON), points(points), faces(faces) { }
	virtual std::string toString() const;
	virtual std::string name() const { return "polyhedron"; }
	virtual const Geometry *createGeometry() const;

	ValuePtr points, faces;
};

class SquareNode : public PrimitiveNode
{
public:
	VISITABLE();
	SquareNode(const ModuleInstantiation *mi, double x, double y, bool c) : PrimitiveNode(mi, SQUARE), x(x), y(y), center(c) { }
	virtual std::string toString() const;
	virtual std::string name() const { return "square"; }
	virtual const Geometry *createGeometry() const;

	double x,y;
	bool center;
};

class CircleNode : public PrimitiveNode
{
public:
	VISITABLE();
	CircleNode(const ModuleInstantiation *mi, double r) : PrimitiveNode(mi, CIRCLE), r(r) { }
	virtual std::string toString() const;
	virtual std::string name() const { return "circle"; }
	virtual const Geometry *createGeometry() const;

	double r;
};

class PolygonNode : public PrimitiveNode
{
public:
	VISITABLE();
	PolygonNode(const ModuleInstantiation *mi, ValuePtr points, ValuePtr paths) : PrimitiveNode(mi, POLYGON), points(points), paths(paths) { }
	virtual std::string toString() const;
	virtual std::string name() const { return "polygon"; }
	virtual const Geometry *createGeometry() const;

	ValuePtr points, paths;
};
