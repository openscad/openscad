#pragma once

#include "glview/Renderer.h"
#include "glview/system-gl.h"
#include <utility>
#include <memory>
#ifdef ENABLE_OPENCSG
#include <opencsg.h>
#endif
#include "core/CSGNode.h"

#include "glview/VBORenderer.h"

#include <cstddef>
#include <vector>

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
  void setCsgObjectIndex(size_t csg_object_index) { csg_object_index_ = csg_object_index; }

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

class OpenCSGVBOProduct
{
public:
  OpenCSGVBOProduct(std::vector<OpenCSG::Primitive *> primitives, std::unique_ptr<std::vector<std::shared_ptr<VertexState>>> states)
    : primitives_(std::move(primitives)), states_(std::move(states)) {}
  virtual ~OpenCSGVBOProduct() = default;

  [[nodiscard]] const std::vector<OpenCSG::Primitive *>& primitives() const { return primitives_; }
  [[nodiscard]] const std::vector<std::shared_ptr<VertexState>>& states() const { return *(states_.get()); }

private:
  // primitives_ is used to create the OpenCSG depth buffer (unlit rendering).
  // states_ is used for color rendering (using GL_EQUAL).
  // Both may use the same underlying VBOs
  const std::vector<OpenCSG::Primitive *> primitives_;
  const std::unique_ptr<std::vector<std::shared_ptr<VertexState>>> states_;
};

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
  OpenCSGVBOPrim *createVBOPrimitive(const std::shared_ptr<OpenCSGVertexState>& vertex_state,
                                     const OpenCSG::Operation operation, const unsigned int convexity) const;
#endif // ENABLE_OPENCSG
  void createCSGVBOProducts(const CSGProducts& products, const Renderer::shaderinfo_t *shaderinfo, bool highlight_mode, bool background_mode);
  void renderCSGVBOProducts(bool showedges, const Renderer::shaderinfo_t *shaderinfo) const;

private:
  std::vector<std::unique_ptr<OpenCSGVBOProduct>> vbo_vertex_products_;
  std::vector<GLuint> all_vbos_;
  std::shared_ptr<CSGProducts> root_products_;
  std::shared_ptr<CSGProducts> highlights_products_;
  std::shared_ptr<CSGProducts> background_products_;
};
