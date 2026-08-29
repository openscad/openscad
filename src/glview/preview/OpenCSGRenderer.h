#pragma once

#include "glview/VertexState.h"
#include "glview/ShaderUtils.h"
#include "geometry/linalg.h"
#include "glview/Renderer.h"
#include "glview/system-gl.h"
#include <utility>
#include <memory>
#ifdef ENABLE_OPENCSG
#include <opencsg.h>
#endif
#include "core/CSGNode.h"

#include "glview/VBORenderer.h"
#include "glview/VBOBuilder.h"

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

class OpenCSGVertexState : public VertexState
{
public:
  OpenCSGVertexState(size_t csg_object_index = 0) : csg_object_index_(csg_object_index) {}
  OpenCSGVertexState(GLenum draw_mode, GLsizei draw_size, GLenum draw_type, size_t draw_offset,
                     size_t element_offset, GLuint vertices_vbo, GLuint elements_vbo,
                     size_t csg_object_index = 0)
    : VertexState(draw_mode, draw_size, draw_type, draw_offset, element_offset, vertices_vbo,
                  elements_vbo),
      csg_object_index_(csg_object_index)
  {
  }

  [[nodiscard]] size_t csgObjectIndex() const { return csg_object_index_; }
  void setCsgObjectIndex(size_t csg_object_index) { csg_object_index_ = csg_object_index; }

private:
  size_t csg_object_index_;
};

class OpenCSGVertexStateFactory : public VertexStateFactory
{
public:
  OpenCSGVertexStateFactory() = default;

  [[nodiscard]] std::shared_ptr<VertexState> createVertexState(GLenum draw_mode, size_t draw_size,
                                                               GLenum draw_type, size_t draw_offset,
                                                               size_t element_offset,
                                                               GLuint vertices_vbo,
                                                               GLuint elements_vbo) const override
  {
    return std::make_shared<OpenCSGVertexState>(draw_mode, draw_size, draw_type, draw_offset,
                                                element_offset, vertices_vbo, elements_vbo);
  }
};

class OpenCSGVBOProduct : public VertexStateContainer
{
public:
  OpenCSGVBOProduct() = default;
  OpenCSGVBOProduct(const OpenCSGVBOProduct& o) = delete;
  OpenCSGVBOProduct(OpenCSGVBOProduct&& o) = delete;
  virtual ~OpenCSGVBOProduct() = default;

  void addPrimitive(std::unique_ptr<OpenCSG::Primitive> p)
  {
    primitives_.push_back(p.get());
    owned_primitives_.push_back(std::move(p));
  }

  [[nodiscard]] const std::vector<OpenCSG::Primitive *>& primitives() const { return primitives_; }

private:
  // primitives_ is used to create the OpenCSG depth buffer (unlit rendering).
  // states_ is used for color rendering (using GL_EQUAL).
  // Both may use the same underlying VBOs
  std::vector<OpenCSG::Primitive *> primitives_;
  std::vector<std::unique_ptr<OpenCSG::Primitive>> owned_primitives_;
};

class OpenCSGRenderer : public VBORenderer
{
public:
  // The preparation phase, split so most of it can run off the GUI thread.
  //
  // beginPrepare() and finishPrepare() make GL calls and must run on the thread that
  // owns the context, with it current. buildProducts() is pure CPU work over the CSG
  // leaves -- ~85% of the phase -- and touches no GL and no widgets, so a window can run
  // it on a worker thread and stop blocking every other window's preparation.
  struct PendingProduct {
    std::unique_ptr<OpenCSGVBOProduct> container;
    std::unique_ptr<VBOBuilder> builder;
    const CSGProduct *product{nullptr};
    bool highlight_mode{false};
    bool background_mode{false};
    const ShaderUtils::ShaderInfo *shaderinfo{nullptr};
  };
  OpenCSGRenderer(std::shared_ptr<CSGProducts> root_products,
                  std::shared_ptr<CSGProducts> highlights_products,
                  std::shared_ptr<CSGProducts> background_products);
  ~OpenCSGRenderer() override = default;
  void prepare(const ShaderUtils::ShaderInfo *shaderinfo = nullptr) override;
  bool prepare(const ShaderUtils::ShaderInfo *shaderinfo, const std::function<bool()>& shouldContinue);

  // GL, context must be current.
  std::vector<PendingProduct> beginPrepare(const ShaderUtils::ShaderInfo *shaderinfo);
  // No GL, no widgets: safe on a worker thread. shouldContinue() is polled between leaves
  // and must be thread-safe -- it must not touch Qt widgets or pump the event loop.
  bool buildProducts(std::vector<PendingProduct>& pending, const std::function<bool()>& shouldContinue);
  // GL, context must be current. Consumes pending.
  void finishPrepare(std::vector<PendingProduct>& pending);
  void draw(bool showedges, const ShaderUtils::ShaderInfo *shaderinfo = nullptr) const override;

  BoundingBox getBoundingBox() const override;

private:
  bool buildProduct(PendingProduct& pending, const std::function<bool()>& shouldContinue);

  std::vector<std::unique_ptr<OpenCSGVBOProduct>> vertex_state_containers_;
  // The shader the containers above were prepared for; see beginPrepare().
  const ShaderUtils::ShaderInfo *prepared_shaderinfo_ = nullptr;
  std::shared_ptr<CSGProducts> root_products_;
  std::shared_ptr<CSGProducts> highlights_products_;
  std::shared_ptr<CSGProducts> background_products_;
  std::string opencsg_vertex_shader_code_;
};
