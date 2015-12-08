#pragma once

#include "renderer.h"
#include "csgterm.h"
#include <boost/unordered_map.hpp>

class ThrownTogetherRenderer : public Renderer
{
public:
	ThrownTogetherRenderer(class CSGProducts *root_products,
												 CSGProducts *highlight_products,
												 CSGProducts *background_products);
	virtual void draw(bool showfaces, bool showedges) const;
	virtual BoundingBox getBoundingBox() const;
private:
	void renderCSGProducts(CSGProducts *products, bool highlight_mode, bool background_mode, bool showedges, 
											bool fberror) const;
	void renderChainObject(const class CSGChainObject &csgobj, bool highlight_mode,
												 bool background_mode, bool showedges, bool fberror, OpenSCADOperator type) const;

	CSGProducts *root_products;
	CSGProducts *highlight_products;
	CSGProducts *background_products;
	mutable boost::unordered_map<std::pair<const Geometry*,const Transform3d*>,int> geomVisitMark;
};
