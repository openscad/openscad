#ifndef OPENCSGRENDERER_H_
#define OPENCSGRENDERER_H_

#include "renderer.h"
#include "system-gl.h"

class OpenCSGRenderer : public Renderer
{
public:
	OpenCSGRenderer(class CSGChain *root_chain, CSGChain *highlights_chain, 
									CSGChain *background_chain, GLint *shaderinfo);
	void draw(bool showfaces, bool showedges) const;
private:
	void renderCSGChain(class CSGChain *chain, GLint *shaderinfo, 
											bool highlight, bool background) const;

	CSGChain *root_chain;
	CSGChain *highlights_chain;
	CSGChain *background_chain;
	GLint *shaderinfo;
};

#endif
