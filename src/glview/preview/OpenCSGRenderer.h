#pragma once

#include "Renderer.h"
#include "system-gl.h"
#ifdef ENABLE_OPENCSG
#include <opencsg.h>
#endif
#include "CSGNode.h"

#include "VBORenderer.h"

class CSGChainObject;
class CSGProducts;
class OpenCSGPrim;
class OpenCSGVBOPrim;

class OpenCSGVertexState : public VertexState
{
public:
  OpenCSGVertexState()
    : VertexState(), csg_object_index_(0)
  {}
  OpenCSGVertexState(GLenum draw_mode, GLsizei draw_size, GLenum draw_type,
                     size_t draw_offset, size_t element_offset, GLuint vertices_vbo, GLuint elements_vbo)
    : VertexState(draw_mode, draw_size, draw_type, draw_offset, element_offset, vertices_vbo, elements_vbo),
    csg_object_index_(0)
  {}
  OpenCSGVertexState(size_t csg_object_index = 0)
    : VertexState(),
    csg_object_index_(csg_object_index)
  {}
  OpenCSGVertexState(GLenum draw_mode, GLsizei draw_size, GLenum draw_type,
                     size_t draw_offset, size_t element_offset, GLuint vertices_vbo, GLuint elements_vbo,
                     size_t csg_object_index)
    : VertexState(draw_mode, draw_size, draw_type, draw_offset, element_offset, vertices_vbo, elements_vbo),
    csg_object_index_(csg_object_index)
  {}

  [[nodiscard]] size_t csgObjectIndex() const { return csg_object_index_; }
  void csgObjectIndex(size_t csg_object_index) { csg_object_index_ = csg_object_index; }

private:
  size_t csg_object_index_;
};

class OpenCSGVertexStateFactory : public VertexStateFactory
{
public:
  OpenCSGVertexStateFactory() = default;

  [[nodiscard]] std::shared_ptr<VertexState> createVertexState(GLenum draw_mode, size_t draw_size, GLenum draw_type,
                                                               size_t draw_offset, size_t element_offset,
                                                               GLuint vertices_vbo, GLuint elements_vbo) const override {
    return std::make_shared<OpenCSGVertexState>(draw_mode, draw_size, draw_type, draw_offset, element_offset, vertices_vbo, elements_vbo);
  }
};

using OpenCSGPrimitives = std::vector<OpenCSG::Primitive *>;
class OpenCSGVBOProduct
{
public:
  OpenCSGVBOProduct(std::unique_ptr<OpenCSGPrimitives> primitives, std::unique_ptr<VertexStates> states)
    : primitives_(std::move(primitives)), states_(std::move(states)) {}
  virtual ~OpenCSGVBOProduct() = default;

  [[nodiscard]] const OpenCSGPrimitives& primitives() const { return *(primitives_.get()); }
  [[nodiscard]] const VertexStates& states() const { return *(states_.get()); }

private:
  const std::unique_ptr<OpenCSGPrimitives> primitives_;
  const std::unique_ptr<VertexStates> states_;
};
using OpenCSGVBOProducts = std::vector<std::unique_ptr<OpenCSGVBOProduct>>;

class OpenCSGRenderer : public VBORenderer
{
public:
  OpenCSGRenderer(std::shared_ptr<CSGProducts> root_products,
                  std::shared_ptr<CSGProducts> highlights_products,
                  std::shared_ptr<CSGProducts> background_products);
  ~OpenCSGRenderer() override {
    if (all_vbos_.size()) {
      glDeleteBuffers(all_vbos_.size(), all_vbos_.data());
    }
  }
  void prepare(bool showfaces, bool showedges, const shaderinfo_t *shaderinfo = nullptr) override;
  void draw(bool showfaces, bool showedges, const shaderinfo_t *shaderinfo = nullptr) const override;

  BoundingBox getBoundingBox() const override;
private:
#ifdef ENABLE_OPENCSG
  OpenCSGPrim *createCSGPrimitive(const CSGChainObject& csgobj, OpenCSG::Operation operation, bool highlight_mode, bool background_mode, OpenSCADOperator type) const;
  OpenCSGVBOPrim *createVBOPrimitive(const std::shared_ptr<OpenCSGVertexState>& vertex_state,
                                     const OpenCSG::Operation operation, const unsigned int convexity) const;
#endif // ENABLE_OPENCSG
  void createCSGProducts(const CSGProducts& products, const Renderer::shaderinfo_t *shaderinfo, bool highlight_mode, bool background_mode);
  void renderCSGProducts(const std::shared_ptr<CSGProducts>& products, bool showedges = false, const Renderer::shaderinfo_t *shaderinfo = nullptr,
                         bool highlight_mode = false, bool background_mode = false) const;

  OpenCSGVBOProducts vbo_vertex_products;
  std::vector<GLuint> all_vbos_;
  std::shared_ptr<CSGProducts> root_products;
  std::shared_ptr<CSGProducts> highlights_products;
  std::shared_ptr<CSGProducts> background_products;
};
