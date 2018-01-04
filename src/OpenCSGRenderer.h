#pragma once

#include "renderer.h"
#include "system-gl.h"
#ifdef ENABLE_OPENCSG
#include <opencsg.h>
#endif
#include "csgnode.h"

class OpenCSGRenderer : public Renderer
{
public:
	OpenCSGRenderer(shared_ptr<class CSGProducts> root_products,
									shared_ptr<CSGProducts> highlights_products,
									shared_ptr<CSGProducts> background_products,
									GLint *shaderinfo);
	void draw(bool showfaces, bool showedges) const override;
	BoundingBox getBoundingBox() const override;
private:
#ifdef ENABLE_OPENCSG
	class OpenCSGPrim *createCSGPrimitive(const class CSGChainObject &csgobj, OpenCSG::Operation operation, bool highlight_mode, bool background_mode, OpenSCADOperator type) const;
#endif
	void renderCSGProducts(const class CSGProducts &products, GLint *shaderinfo, 
											bool highlight_mode, bool background_mode) const;

	shared_ptr<CSGProducts> root_products;
	shared_ptr<CSGProducts> highlights_products;
	shared_ptr<CSGProducts> background_products;
	GLint *shaderinfo;
};
