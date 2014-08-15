#pragma once

#include "renderer.h"
#include "CGAL_Nef_polyhedron.h"

class CGALRenderer : public Renderer
{
public:
	CGALRenderer(shared_ptr<const class Geometry> geom);
	~CGALRenderer();
	virtual void draw(bool showfaces, bool showedges) const;
	virtual void setColorScheme(const ColorScheme &cs);
	virtual BoundingBox getBoundingBox() const;

private:
	shared_ptr<class CGAL_OGL_Polyhedron> getPolyhedron() const;
	void buildPolyhedron() const;

	mutable shared_ptr<class CGAL_OGL_Polyhedron> polyhedron;
	shared_ptr<const CGAL_Nef_polyhedron> N;
	shared_ptr<const class PolySet> polyset;
};
