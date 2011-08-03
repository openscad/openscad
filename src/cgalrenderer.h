#ifndef CGALRENDERER_H_
#define CGALRENDERER_H_

#include "renderer.h"
#include "cgal.h"

class CGALRenderer : public Renderer
{
public:
	CGALRenderer(const CGAL_Nef_polyhedron &root);
	~CGALRenderer();
	void draw(bool showfaces, bool showedges) const;

private:
	const CGAL_Nef_polyhedron &root;
	class Polyhedron *polyhedron;
	class PolySet *polyset;
};

#endif
