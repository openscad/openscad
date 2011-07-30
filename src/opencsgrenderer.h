#ifndef OPENCSGRENDERER_H_
#define OPENCSGRENDERER_H_

#include "renderer.h"
#include <GL/glew.h> // this must be included before the GL headers
#include <qgl.h>

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
