#pragma once

#include "cgal.h"

#include "VBORenderer.h"
#include "CGAL_OGL_Polyhedron.h"
#include "CGAL_OGL_VBOPolyhedron.h"
#include "CGAL_Nef_polyhedron.h"

class CGALRenderer : public VBORenderer
{
public:
	CGALRenderer(shared_ptr<const class Geometry> geom);
	~CGALRenderer();
	void draw(bool showfaces, bool showedges) const override;
	void setColorScheme(const ColorScheme &cs) override;
	BoundingBox getBoundingBox() const override;

public:
	mutable std::list<shared_ptr<class CGAL_OGL_Polyhedron> > polyhedrons;
	std::list<shared_ptr<const class PolySet> > polysets;
	std::list<shared_ptr<const CGAL_Nef_polyhedron> > nefPolyhedrons;

private:
	void addGeometry(const shared_ptr<const class Geometry> &geom);
	const std::list<shared_ptr<class CGAL_OGL_Polyhedron> > &getPolyhedrons() const;
	void buildPolyhedrons() const;
	void createPolysets() const;
	bool last_render_state; // FIXME: this is temporary to make switching between renderers seamless.
	
	mutable VBORenderer::ProductVertexSets product_vertex_sets;
};
