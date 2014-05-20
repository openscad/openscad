#pragma once

#include "renderer.h"

class CGALRenderer : public Renderer
{
public:
	CGALRenderer(shared_ptr<const class Geometry> geom);
	~CGALRenderer();
	void draw(bool showfaces, bool showedges) const;

public:
	class Polyhedron *polyhedron;
	shared_ptr<const class PolySet> polyset;
};
