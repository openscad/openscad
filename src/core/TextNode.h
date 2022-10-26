#pragma once

#include "node.h"
#include "Value.h"

#include "FreetypeRenderer.h"

class TextModule;

class TextNode : public AbstractPolyNode
{
public:
  VISITABLE();
  TextNode(const ModuleInstantiation *mi) : AbstractPolyNode(mi) {}

  void print(scad::ostringstream& stream) const override final;
  std::string name() const override final { return "text"; }

  virtual std::vector<const class Geometry *> createGeometryList() const;

  virtual FreetypeRenderer::Params get_params() const;

  FreetypeRenderer::Params params;
};
