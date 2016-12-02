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
	VISITABLE();
	PrimitiveNode(const ModuleInstantiation *mi, primitive_type_e type) : LeafNode(mi), type(type) { }
	virtual std::string toString() const;
	virtual std::string name() const {
		switch (this->type) {
		case CUBE:
			return "cube";
			break;
		case SPHERE:
			return "sphere";
			break;
		case CYLINDER:
			return "cylinder";
			break;
		case POLYHEDRON:
			return "polyhedron";
			break;
		case SQUARE:
			return "square";
			break;
		case CIRCLE:
			return "circle";
			break;
		case POLYGON:
			return "polygon";
			break;
		default:
			assert(false && "PrimitiveNode::name(): Unknown primitive type");
			return "unknown";
		}
	}

	bool center;
	double x, y, z, h, r1, r2;
	double fn, fs, fa;
	primitive_type_e type;
	int convexity;
	ValuePtr points, paths, faces;
	virtual const Geometry *createGeometry() const;
};

