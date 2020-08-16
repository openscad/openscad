#pragma once

#include "renderer.h"
#include "csgnode.h"
#include <unordered_map>
#include <boost/functional/hash.hpp>

#include "VBORenderer.h"

class ThrownTogetherRenderer : public VBORenderer
{
public:
	ThrownTogetherRenderer(shared_ptr<class CSGProducts> root_products,
				shared_ptr<CSGProducts> highlight_products,
				shared_ptr<CSGProducts> background_products);
  virtual ~ThrownTogetherRenderer();
	void draw(bool showfaces, bool showedges) const override;
	void draw_with_shader(const Renderer::shaderinfo_t *, bool showedges = false) const;

	BoundingBox getBoundingBox() const override;
private:
	void createCSGProducts(const CSGProducts &products, bool highlight_mode, bool background_mode) const;
	void renderCSGProducts(const shared_ptr<CSGProducts> &products, const Renderer::shaderinfo_t *, bool highlight_mode = false, bool background_mode = false, bool showedges = false,
				bool fberror = false) const;
	void createChainObject(VertexSets &vertex_sets, std::vector<Vertex> &render_buffer,
				const class CSGChainObject &csgobj, bool highlight_mode, bool background_mode,
				bool fberror, OpenSCADOperator type) const;
	void renderChainObject(const class CSGChainObject &csgobj, const Renderer::shaderinfo_t *, bool highlight_mode,
				bool background_mode, bool showedges, bool fberror, OpenSCADOperator type) const;

	Renderer::ColorMode getColorMode(const CSGNode::Flag &flags, bool highlight_mode,
	                                 bool background_mode, bool fberror, OpenSCADOperator type) const;
					 
	inline const shared_ptr<ProductVertexSets> &getProductVertexSets() const { return product_vertex_sets; }

	shared_ptr<ProductVertexSets> product_vertex_sets;
	shared_ptr<CSGProducts> root_products;
	shared_ptr<CSGProducts> highlight_products;
	shared_ptr<CSGProducts> background_products;
	mutable std::unordered_map<std::pair<const Geometry*,const Transform3d*>, int,
				   boost::hash<std::pair<const Geometry*,const Transform3d*>>> geomVisitMark;
};
