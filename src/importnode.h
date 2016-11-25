#pragma once

#include "node.h"
#include "value.h"

enum import_type_e {
	TYPE_UNKNOWN,
	TYPE_AMF,
	TYPE_STL,
	TYPE_OFF,
	TYPE_SVG,
	TYPE_DXF,
	TYPE_NEF3,
};

class ImportNode : public LeafNode
{
public:
	VISITABLE();
	ImportNode(const ModuleInstantiation *mi, import_type_e type) : LeafNode(mi), type(type) { }
	virtual std::string toString() const;
	virtual std::string name() const;

	import_type_e type;
	Filename filename;
	std::string layername;
	int convexity;
	double fn, fs, fa;
	double origin_x, origin_y, scale;
	double width, height;
	virtual const class Geometry *createGeometry() const;
};
