#pragma once

#include "renderer.h"
#include "system-gl.h"
#ifdef ENABLE_OPENCSG
#include <opencsg.h>
#endif
#include "csgterm.h"

class OpenCSGRenderer : public Renderer
{
public:
	OpenCSGRenderer(class CSGProducts *root_products, CSGProducts *highlights_products, 
									CSGProducts *background_products, GLint *shaderinfo);
	virtual void draw(bool showfaces, bool showedges) const;
	virtual BoundingBox getBoundingBox() const;
private:
#ifdef ENABLE_OPENCSG
	class OpenCSGPrim *createCSGPrimitive(const class CSGChainObject &csgobj, OpenCSG::Operation operation, bool highlight_mode, bool background_mode, CSGOperation::type_e type) const;
#endif
	void renderCSGProducts(const class CSGProducts &products, GLint *shaderinfo, 
											bool highlight_mode, bool background_mode) const;

	CSGProducts *root_products;
	CSGProducts *highlights_products;
	CSGProducts *background_products;
	GLint *shaderinfo;
};
