#pragma once

#include "renderer.h"

class ThrownTogetherRenderer : public Renderer
{
public:
	ThrownTogetherRenderer(class CSGChain *root_chain,
												 CSGChain *highlights_chain, CSGChain *background_chain);
	virtual void draw(bool showfaces, bool showedges) const;
	virtual BoundingBox getBoundingBox() const;
private:
	void renderCSGChain(CSGChain *chain, bool highlight, bool background, bool showedges, 
											bool fberror) const;

	CSGChain *root_chain;
	CSGChain *highlights_chain;
	CSGChain *background_chain;
};
