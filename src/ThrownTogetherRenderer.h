#pragma once

#include "renderer.h"

class ThrownTogetherRenderer : public Renderer
{
public:
	ThrownTogetherRenderer(class CSGChain *root_chain,
												 CSGChain *highlights_chain,
												 CSGChain *background_chain);
	ThrownTogetherRenderer(class CSGChain *root_chain, class CSGProducts *root_products,
												 CSGChain *highlights_chain, CSGProducts *highlight_products,
												 CSGChain *background_chain, CSGProducts *background_products);
	virtual void draw(bool showfaces, bool showedges) const;
	virtual BoundingBox getBoundingBox() const;
private:
	void renderCSGChain(CSGChain *chain, bool highlight, bool background, bool showedges, 
											bool fberror) const;

	CSGChain *root_chain;
	CSGChain *highlights_chain;
	CSGChain *background_chain;
	CSGProducts *root_products;
	CSGProducts *highlights_products;
	CSGProducts *background_products;
};
