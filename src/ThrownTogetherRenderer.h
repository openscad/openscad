#pragma once

#include "renderer.h"
#include "csgnode.h"
#include <unordered_map>
#include <boost/functional/hash.hpp>

#ifdef ENABLE_EXPERIMENTAL
#include "VBORenderer.h"
#endif // ENABLE_EXPERIMENTAL

#ifdef ENABLE_EXPERIMENTAL
class ThrownTogetherRenderer : public VBORenderer
#else
class ThrownTogetherRenderer : public Renderer
#endif // ENABLE_EXPERIMENTAL
{
public:
	ThrownTogetherRenderer(shared_ptr<class CSGProducts> root_products,
				shared_ptr<CSGProducts> highlight_products,
				shared_ptr<CSGProducts> background_products);
  virtual ~ThrownTogetherRenderer();
	void draw(bool showfaces, bool showedges) const override;
	void draw_with_shader(const GLView::shaderinfo_t *shaderinfo) const override {
		this->draw_with_shader(shaderinfo, false);
	}
	void draw_with_shader(const GLView::shaderinfo_t *, bool showedges) const;

	BoundingBox getBoundingBox() const override;
private:
	void renderCSGProducts(const CSGProducts &products, const GLView::shaderinfo_t *, bool highlight_mode, bool background_mode, bool showedges,
				bool fberror) const;
	void renderChainObject(const class CSGChainObject &csgobj, const GLView::shaderinfo_t *, bool highlight_mode,
				bool background_mode, bool showedges, bool fberror, OpenSCADOperator type) const;

#ifdef ENABLE_EXPERIMENTAL
	Renderer::ColorMode getColorMode(const CSGNode::Flag &flags, bool highlight_mode,
	                                 bool background_mode, bool fberror, OpenSCADOperator type) const;
					 
	void createChainObject(std::vector<VertexSet *> &vertex_sets, std::vector<Vertex> &render_buffer,
				const class CSGChainObject &csgobj, bool highlight_mode, bool background_mode,
				bool fberror, OpenSCADOperator type) const;
	
	void drawCSGProducts(bool use_edge_shader) const;
	inline const std::vector<VertexSets *> &getProductVertexSets() const { return *product_vertex_sets; }

	std::vector<VertexSets *> *product_vertex_sets;
#endif // ENABLE_EXPERIMENTAL

	shared_ptr<CSGProducts> root_products;
	shared_ptr<CSGProducts> highlight_products;
	shared_ptr<CSGProducts> background_products;
	mutable std::unordered_map<std::pair<const Geometry*,const Transform3d*>, int,
				   boost::hash<std::pair<const Geometry*,const Transform3d*>>> geomVisitMark;
};
