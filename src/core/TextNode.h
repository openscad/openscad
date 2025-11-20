#pragma once

#include <memory>
#include <string>
#include <vector>

#include "core/node.h"
#include "core/ModuleInstantiation.h"
#include "core/FreetypeRenderer.h"
#include "geometry/Polygon2d.h"

class TextModule;

class TextNode : public AbstractPolyNode
{
public:
  VISITABLE();
  TextNode(const ModuleInstantiation *mi, FreetypeRenderer::Params&& p)
    : AbstractPolyNode(mi), params(std::move(p))
  {
  }

  std::string toString() const override;
  std::string name() const override { return "text"; }

  std::vector<std::shared_ptr<const Polygon2d>> createPolygonList() const;

  FreetypeRenderer::Params params;
};
