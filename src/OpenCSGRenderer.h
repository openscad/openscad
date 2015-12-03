#pragma once

#include "renderer.h"
#include "system-gl.h"
#ifdef ENABLE_OPENCSG
#include <opencsg.h>
#endif

class OpenCSGRenderer : public Renderer
{
public:
	OpenCSGRenderer(class CSGChain *root_chain, CSGChain *highlights_chain, 
									CSGChain *background_chain, GLint *shaderinfo);
	OpenCSGRenderer(class CSGChain *root_chain, class CSGProducts *root_products, CSGChain *highlights_chain, CSGProducts *highlights_products, 
									CSGChain *background_chain, CSGProducts *background_products, GLint *shaderinfo);
	virtual void draw(bool showfaces, bool showedges) const;
	virtual BoundingBox getBoundingBox() const;
private:
#ifdef ENABLE_OPENCSG
	class OpenCSGPrim *createCSGPrimitive(const class CSGChainObject &csgobj, OpenCSG::Operation operation, bool highlight, bool background) const;
#endif
	void renderCSGChain(class CSGChain *chain, GLint *shaderinfo, 
											bool highlight, bool background) const;
	void renderCSGProducts(const class CSGProducts &products, GLint *shaderinfo, 
											bool highlight, bool background) const;

	CSGChain *root_chain;
	CSGProducts *root_products;
	CSGChain *highlights_chain;
	CSGChain *background_chain;
	CSGProducts *highlights_products;
	CSGProducts *background_products;
	GLint *shaderinfo;
};
