#pragma once

#include "node.h"
#include "value.h"

#include "FreetypeRenderer.h"

class TextModule;

class TextNode : public AbstractPolyNode
{
public:
	VISITABLE();
	TextNode(const ModuleInstantiation *mi) : AbstractPolyNode(mi) {}
	
	std::string toString() const override;
	std::string name() const override { return "text"; }
	
	virtual std::vector<const class Geometry *> createGeometryList() const;
  
	virtual FreetypeRenderer::Params get_params() const;
private:
	FreetypeRenderer::Params params;
	friend class TextModule;
};
