#ifndef THROWNTOGETHERRENDERER_H_
#define THROWNTOGETHERRENDERER_H_

#include "renderer.h"

class ThrownTogetherRenderer : public Renderer
{
public:
	ThrownTogetherRenderer(class CSGChain *root_chain,
												 CSGChain *highlights_chain, CSGChain *background_chain);
	void draw(bool showfaces, bool showedges) const;
private:
	void renderCSGChain(CSGChain *chain, bool highlight, bool background, bool showedges, 
											bool fberror) const;

	CSGChain *root_chain;
	CSGChain *highlights_chain;
	CSGChain *background_chain;
};

#endif
