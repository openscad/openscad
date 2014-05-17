#pragma once

#include "renderer.h"

class CGALRenderer : public Renderer
{
public:
	CGALRenderer(shared_ptr<const class Geometry> geom);
	~CGALRenderer();
	void draw(bool showfaces, bool showedges) const;
	BoundingBox getBoundingBox() const;

public:
	std::list<shared_ptr<class Polyhedron> > polyhedrons;
	std::list<shared_ptr<const class PolySet> > polysets;

private:
	void addGeometry(const shared_ptr<const class Geometry> &geom);
};
