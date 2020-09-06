#pragma once

#include "renderer.h"
#include "csgnode.h"
#include <unordered_map>
#include <boost/functional/hash.hpp>

#include "VBORenderer.h"

class TTRVertexState : public VertexState
{
public:
	TTRVertexState()
		: VertexState(), csg_object_index_(0)
	{}
	TTRVertexState(size_t csg_object_index)
		: VertexState(), csg_object_index_(csg_object_index)
	{}
	TTRVertexState(GLenum draw_type, GLsizei draw_size, size_t draw_offset = 0, size_t csg_object_index = 0)
		: VertexState(draw_type, draw_size, draw_offset), csg_object_index_(csg_object_index)
	{}
	virtual ~TTRVertexState() {}

	size_t csgObjectIndex() const { return csg_object_index_; }
	void csgObjectIndex(size_t csg_object_index) { csg_object_index_ = csg_object_index; }

private:
	size_t csg_object_index_;
};

class TTRVertexStateFactory : public VertexStateFactory {
public:
	TTRVertexStateFactory() {}
	virtual ~TTRVertexStateFactory() {}
	
	std::shared_ptr<VertexState> createVertexState(GLenum draw_type, size_t draw_size, size_t draw_offset = 0) const override {
		return std::make_shared<TTRVertexState>(draw_type, draw_size, draw_offset);
	}
};

class ThrownTogetherRenderer : public VBORenderer
{
public:
	ThrownTogetherRenderer(shared_ptr<class CSGProducts> root_products,
				shared_ptr<CSGProducts> highlight_products,
				shared_ptr<CSGProducts> background_products);
  virtual ~ThrownTogetherRenderer();
	void draw(bool showfaces, bool showedges, const Renderer::shaderinfo_t *shaderinfo = nullptr) const override;

	BoundingBox getBoundingBox() const override;
private:
	void renderCSGProducts(const shared_ptr<CSGProducts> &products, bool showedges = false, 
				const Renderer::shaderinfo_t * shaderinfo = nullptr,
				bool highlight_mode = false, bool background_mode = false,
				bool fberror = false) const;
	void renderChainObject(const class CSGChainObject &csgobj, bool showedges,
				const Renderer::shaderinfo_t *, bool highlight_mode,
				bool background_mode, bool fberror, OpenSCADOperator type) const;

	void createCSGProducts(const CSGProducts &products, VertexArray &vertex_array,
				bool highlight_mode, bool background_mode) const;
	void createChainObject(VertexArray &vertex_array, const class CSGChainObject &csgobj,
				bool highlight_mode, bool background_mode,
				OpenSCADOperator type) const;

	Renderer::ColorMode getColorMode(const CSGNode::Flag &flags, bool highlight_mode,
	                                 bool background_mode, bool fberror, OpenSCADOperator type) const;

	mutable VertexStates vertex_states;
	mutable GLuint vbo;
	shared_ptr<CSGProducts> root_products;
	shared_ptr<CSGProducts> highlight_products;
	shared_ptr<CSGProducts> background_products;
	mutable std::unordered_map<std::pair<const Geometry*,const Transform3d*>, int,
				   boost::hash<std::pair<const Geometry*,const Transform3d*>>> geomVisitMark;
};
