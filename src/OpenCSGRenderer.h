#pragma once

#include "renderer.h"
#include "system-gl.h"
#ifdef ENABLE_OPENCSG
#include <opencsg.h>
#endif
#include "csgnode.h"

#ifdef ENABLE_EXPERIMENTAL
#include "VBORenderer.h"
#endif // ENABLE_EXPERIMENTAL

#ifdef ENABLE_EXPERIMENTAL
class OpenCSGRenderer : public VBORenderer
#else
class OpenCSGRenderer : public Renderer
#endif // ENABLE_EXPERIMENTAL
{
public:
	OpenCSGRenderer(shared_ptr<class CSGProducts> root_products,
									shared_ptr<CSGProducts> highlights_products,
									shared_ptr<CSGProducts> background_products,
									GLView::shaderinfo_t *shaderinfo);
	virtual ~OpenCSGRenderer();
	void draw(bool showfaces, bool showedges) const override;
	void draw_with_shader(const GLView::shaderinfo_t *shaderinfo) const override;

	BoundingBox getBoundingBox() const override;
private:
#ifdef ENABLE_OPENCSG
#ifdef ENABLE_EXPERIMENTAL
	class OpenCSGPrim *createCSGPrimitive(const VertexSet &vertex_set, const GLuint vbo) const;
#else
	class OpenCSGPrim *createCSGPrimitive(const class CSGChainObject &csgobj, OpenCSG::Operation operation, bool highlight_mode, bool background_mode, OpenSCADOperator type) const;
#endif // ENABLE_EXPERIMENTAL
#endif // ENABLE_OPENCSG
	void renderCSGProducts(const class CSGProducts &products, const GLView::shaderinfo_t *shaderinfo,
				bool highlight_mode, bool background_mode) const;
#ifdef ENABLE_EXPERIMENTAL
	void drawCSGProducts(bool use_edge_shader) const;
	inline const std::vector<VertexSets *> &getProductVertexSets() const { return *product_vertex_sets; }

	std::vector<VertexSets *> *product_vertex_sets;
#endif // ENABLE_EXPERIMENTAL

	shared_ptr<CSGProducts> root_products;
	shared_ptr<CSGProducts> highlights_products;
	shared_ptr<CSGProducts> background_products;
	GLView::shaderinfo_t *shaderinfo;
};
