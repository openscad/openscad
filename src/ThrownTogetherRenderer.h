#pragma once

#include "renderer.h"
#include "csgnode.h"

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
	TTRVertexState(GLenum draw_mode, GLsizei draw_size, GLenum draw_type,
			size_t draw_offset, size_t element_offset, GLuint vertices_vbo, GLuint elements_vbo,
			size_t csg_object_index = 0)
		: VertexState(draw_mode, draw_size, draw_type, draw_offset, element_offset, vertices_vbo, elements_vbo), csg_object_index_(csg_object_index)
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
	
	std::shared_ptr<VertexState> createVertexState(GLenum draw_mode, size_t draw_size, GLenum draw_type,
							size_t draw_offset, size_t element_offset,
							GLuint vertices_vbo, GLuint elements_vbo) const override {
		return std::make_shared<TTRVertexState>(draw_mode, draw_size, draw_type, draw_offset, element_offset, vertices_vbo, elements_vbo);
	}
};

class ThrownTogetherRenderer : public VBORenderer
{
public:
	ThrownTogetherRenderer(shared_ptr<class CSGProducts> root_products,
				shared_ptr<CSGProducts> highlight_products,
				shared_ptr<CSGProducts> background_products);
	virtual ~ThrownTogetherRenderer();
	void prepare(bool showfaces, bool showedges, const shaderinfo_t *shaderinfo = nullptr) override;
	void draw(bool showfaces, bool showedges, const shaderinfo_t *shaderinfo = nullptr) const override;

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
				bool highlight_mode, bool background_mode);
	void createChainObject(VertexArray &vertex_array, const class CSGChainObject &csgobj,
				bool highlight_mode, bool background_mode,
				OpenSCADOperator type);

	Renderer::ColorMode getColorMode(const CSGNode::Flag &flags, bool highlight_mode,
	                                 bool background_mode, bool fberror, OpenSCADOperator type) const;

	VertexStates vertex_states;
	shared_ptr<CSGProducts> root_products;
	shared_ptr<CSGProducts> highlight_products;
	shared_ptr<CSGProducts> background_products;
	GLuint vertices_vbo;
	GLuint elements_vbo;
};
