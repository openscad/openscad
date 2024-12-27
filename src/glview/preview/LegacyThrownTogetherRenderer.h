#pragma once

#include "glview/Renderer.h"
#include "core/CSGNode.h"
#include <utility>
#include <memory>
#include <boost/functional/hash.hpp>
#include <unordered_map>

class CSGProducts;
class CSGChainObject;

class LegacyThrownTogetherRenderer : public Renderer
{
public:
  LegacyThrownTogetherRenderer(std::shared_ptr<CSGProducts> root_products,
			       std::shared_ptr<CSGProducts> highlight_products,
			       std::shared_ptr<CSGProducts> background_products);
  ~LegacyThrownTogetherRenderer() override = default;
  void prepare(bool showfaces, bool showedges, const RendererUtils::ShaderInfo *shaderinfo = nullptr) override {}
  void draw(bool showfaces, bool showedges, const RendererUtils::ShaderInfo *shaderinfo = nullptr) const override;

  BoundingBox getBoundingBox() const override;
private:
  void renderCSGProducts(const std::shared_ptr<CSGProducts>& products, bool showedges = false,
                         const RendererUtils::ShaderInfo *shaderinfo = nullptr,
                         bool highlight_mode = false, bool background_mode = false,
                         bool fberror = false) const;
  void renderChainObject(const CSGChainObject& csgobj, bool showedges,
                         const RendererUtils::ShaderInfo *, bool highlight_mode,
                         bool background_mode, bool fberror, OpenSCADOperator type) const;

  Renderer::ColorMode getColorMode(const CSGNode::Flag& flags, bool highlight_mode,
                                   bool background_mode, bool fberror, OpenSCADOperator type) const;

  std::shared_ptr<CSGProducts> root_products;
  std::shared_ptr<CSGProducts> highlight_products;
  std::shared_ptr<CSGProducts> background_products;
  mutable std::unordered_map<std::pair<const PolySet *, const Transform3d *>, int,
                             boost::hash<std::pair<const PolySet *, const Transform3d *>>> geomVisitMark;
};
