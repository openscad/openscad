#pragma once

#include "renderer.h"
class CGALRenderer : public Renderer
{
public:
	CGALRenderer(shared_ptr<const class Geometry> geom);
	~CGALRenderer();
	void draw(bool showfaces, bool showedges) const override;
	void setColorScheme(const ColorScheme &cs) override;
	BoundingBox getBoundingBox() const override;

private:
#ifdef ENABLE_CGALNEF
	shared_ptr<class CGAL_OGL_Polyhedron> getPolyhedron() const;
	void buildPolyhedron() const;

	mutable shared_ptr<class CGAL_OGL_Polyhedron> polyhedron;
	shared_ptr<const class CGAL_Nef_polyhedron> N;
#endif
	shared_ptr<const class PolySet> polyset;
};
