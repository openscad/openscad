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
	std::string toString() const override;
	std::string name() const override;

	ImportType type;
	Filename filename;
	std::string layername;
	int convexity;
	double fn, fs, fa;
	double origin_x, origin_y, scale;
	double width, height;
	const class Geometry *createGeometry() const override;
};
