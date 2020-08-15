#pragma once

#include "renderer.h"
#include "system-gl.h"
#ifdef ENABLE_OPENCSG
#include <opencsg.h>
#endif
#include "csgnode.h"

#include "VBORenderer.h"

class OpenCSGRenderer : public VBORenderer
{
public:
	OpenCSGRenderer(shared_ptr<class CSGProducts> root_products,
			shared_ptr<CSGProducts> highlights_products,
			shared_ptr<CSGProducts> background_products);
	virtual ~OpenCSGRenderer();
	void draw(bool showfaces, bool showedges) const override;
	void draw_with_shader(const Renderer::shaderinfo_t *shaderinfo) const override;

	BoundingBox getBoundingBox() const override;
private:
#ifdef ENABLE_OPENCSG
	class OpenCSGPrim *createCSGPrimitive(const class CSGChainObject &csgobj, OpenCSG::Operation operation, bool highlight_mode, bool background_mode, OpenSCADOperator type) const;
	class OpenCSGVBOPrim *createVBOPrimitive(const VertexSet &vertex_set, const GLuint vbo) const;
#endif // ENABLE_OPENCSG
	void createCSGProducts(const class CSGProducts &products, bool highlight_mode, bool background_mode) const;
	void renderCSGProducts(const shared_ptr<class CSGProducts> products, const Renderer::shaderinfo_t *shaderinfo,
				bool highlight_mode = false, bool background_mode = false) const;

	inline const std::vector<VertexSets *> &getProductVertexSets() const { return *product_vertex_sets; }

	std::vector<VertexSets *> *product_vertex_sets;
	shared_ptr<CSGProducts> root_products;
	shared_ptr<CSGProducts> highlights_products;
	shared_ptr<CSGProducts> background_products;
};
