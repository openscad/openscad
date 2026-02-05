#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include "core/CSGNode.h"
#include "core/enums.h"
#include "geometry/linalg.h"
#include "glview/system-gl.h"
#include "glview/VBORenderer.h"
#include "glview/VertexState.h"
#include "glview/ShaderUtils.h"
#include "glview/VBOBuilder.h"

class CSGProducts;
class CSGChainObject;

class ThrownTogetherRenderer : public VBORenderer
{
public:
  ThrownTogetherRenderer(std::shared_ptr<CSGProducts> root_products,
                         std::shared_ptr<CSGProducts> highlight_products,
                         std::shared_ptr<CSGProducts> background_products);
  ~ThrownTogetherRenderer() override = default;
  void prepare(const ShaderUtils::Shader *shader) override;
  void draw(const ShaderUtils::Shader *shader) const override;

  BoundingBox getBoundingBox() const override;

private:
  void renderCSGProducts(const std::shared_ptr<CSGProducts>& products, bool showedges = false,
                         const ShaderUtils::Shader *shader = nullptr,
                         bool highlight_mode = false, bool background_mode = false,
                         bool fberror = false) const;

  void createCSGProducts(const CSGProducts& products, VertexStateContainer& container,
                         VBOBuilder& vbo_builder, bool highlight_mode, bool background_mode,
                         const ShaderUtils::Shader *shader);
  void createChainObject(VertexStateContainer& container, VBOBuilder& vbo_builder,
                         const CSGChainObject& csgobj, bool highlight_mode, bool background_mode,
                         OpenSCADOperator type, const ShaderUtils::Shader *shader);

  std::shared_ptr<CSGProducts> root_products_;
  std::shared_ptr<CSGProducts> highlight_products_;
  std::shared_ptr<CSGProducts> background_products_;
  std::vector<VertexStateContainer> vertex_state_containers_;
};
