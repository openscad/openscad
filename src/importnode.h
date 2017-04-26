#pragma once

#include "node.h"
#include "value.h"

enum class ImportType {
	UNKNOWN,
	AMF,
	STL,
	OFF,
	SVG,
	DXF,
	NEF3,
};

class ImportNode : public LeafNode
{
public:
	VISITABLE();
	ImportNode(const ModuleInstantiation *mi, ImportType type) : LeafNode(mi), type(type) { }
	virtual std::string toString() const;
	virtual std::string name() const;

	ImportType type;
	Filename filename;
	std::string layername;
	int convexity;
	double fn, fs, fa;
	double origin_x, origin_y, scale;
	double width, height;
	virtual const class Geometry *createGeometry() const;
};
