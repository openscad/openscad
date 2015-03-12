#pragma once

#include "node.h"
#include "visitor.h"

class TraceNode : public LeafNode
{
public:
	TraceNode(const ModuleInstantiation *mi) : LeafNode(mi), fn(0), fs(0), fa(0) { }
        virtual Response accept(class State &state, Visitor &visitor) const {
		return visitor.visit(state, *this);
	}
	virtual std::string toString() const;
	virtual std::string name() const { return "trace"; }
	virtual class Geometry *createGeometry() const;

	double fn, fs, fa, threshold;
        std::string file, fullpath;
private:
	Geometry *traceBitmap(std::vector<unsigned char> &img, unsigned int width, unsigned int height) const;
	Geometry *createDummyGeometry(std::vector<unsigned char> &img, unsigned int width, unsigned int height) const;
};
