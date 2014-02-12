#ifndef CGALRENDERER_H_
#define CGALRENDERER_H_

#include "renderer.h"

class CGALRenderer : public Renderer
{
public:
	CGALRenderer(shared_ptr<const class Geometry> geom);
	~CGALRenderer();
	void draw(bool showfaces, bool showedges) const;
        void setColorScheme( const OSColors::colorscheme &cs );
public:
	class Polyhedron *polyhedron;
	shared_ptr<const class PolySet> polyset;
};

#endif
