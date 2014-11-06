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

public:
	mutable std::list<shared_ptr<class CGAL_OGL_Polyhedron> > polyhedrons;
	std::list<shared_ptr<const class PolySet> > polysets;
	std::list<shared_ptr<const CGAL_Nef_polyhedron> > nefPolyhedrons;

private:
	void addGeometry(const shared_ptr<const class Geometry> &geom);
	const std::list<shared_ptr<class CGAL_OGL_Polyhedron> > &getPolyhedrons() const;
	void buildPolyhedrons() const;
};
