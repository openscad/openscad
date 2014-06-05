#pragma once

#include "node.h"
#include "visitor.h"
#include "value.h"

#include "FreetypeRenderer.h"

class TextModule;

class TextNode : public AbstractPolyNode
{
public:
	TextNode(const ModuleInstantiation *mi) : AbstractPolyNode(mi) {}
	
	virtual Response accept(class State &state, Visitor &visitor) const {
		return visitor.visit(state, *this);
	}
	
	virtual std::string toString() const;
	virtual std::string name() const { return "text"; }
	
	virtual std::vector<const class Geometry *> createGeometryList() const;
  
	virtual FreetypeRenderer::Params get_params() const;
private:
	FreetypeRenderer::Params params;
	friend class TextModule;
};
