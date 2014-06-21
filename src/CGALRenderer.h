#pragma once

#include "renderer.h"

class CGALRenderer : public Renderer
{
public:
	CGALRenderer(shared_ptr<const class Geometry> geom);
	virtual ~CGALRenderer();
	virtual void draw(bool showfaces, bool showedges) const;
	virtual BoundingBox getBoundingBox() const;

public:
	shared_ptr<class Polyhedron> polyhedron;
	shared_ptr<const class PolySet> polyset;
};
