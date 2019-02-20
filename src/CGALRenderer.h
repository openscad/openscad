#pragma once

#include "renderer.h"
#include "CGAL_Nef_polyhedron.h"

class CGALRenderer : public Renderer
{
public:
	CGALRenderer(shared_ptr<const class Geometry> geom);
	~CGALRenderer();
	void draw(bool showfaces, bool showedges) const override;
	void setColorScheme(const ColorScheme &cs) override;
	BoundingBox getBoundingBox() const override;

private:
	shared_ptr<class CGAL_OGL_Polyhedron> getPolyhedron() const;
	void buildPolyhedron() const;

	mutable shared_ptr<class CGAL_OGL_Polyhedron> polyhedron;
	shared_ptr<const CGAL_Nef_polyhedron> N;
	shared_ptr<const class PolySet> polyset;
};
