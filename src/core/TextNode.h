#pragma once

#include "node.h"
#include "FreetypeRenderer.h"
#include "Polygon2d.h"

class TextModule;

class TextNode : public AbstractPolyNode
{
public:
  VISITABLE();
  TextNode(const ModuleInstantiation *mi) : AbstractPolyNode(mi) {}

  std::string toString() const override;
  std::string name() const override { return "text"; }

  std::vector<std::shared_ptr<const Polygon2d>> createPolygonList() const;

  virtual FreetypeRenderer::Params get_params() const;

  FreetypeRenderer::Params params;
};
