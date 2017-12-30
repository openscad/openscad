#pragma once

#include "Geometry.h"
#include "system-gl.h"
#include "linalg.h"
#include "GeometryUtils.h"
#include "renderer.h"
#include "Polygon2d.h"
#include <vector>
#include <string>

#include <boost/logic/tribool.hpp>
BOOST_TRIBOOL_THIRD_STATE(unknown)

class PolySet : public Geometry
{
public:
	Polygons polygons;

	PolySet(unsigned int dim, boost::tribool convex = unknown);
	PolySet(const Polygon2d &origin);
	~PolySet();

	size_t memsize() const override;
	BoundingBox getBoundingBox() const override;
	std::string dump() const override;
	unsigned int getDimension() const override { return this->dim; }
	bool isEmpty() const override { return polygons.size() == 0; }
	Geometry *copy() const override { return new PolySet(*this); }

	void quantizeVertices();
	size_t numPolygons() const { return polygons.size(); }
	void append_poly();
	void append_poly(const Polygon &poly);
	void append_vertex(double x, double y, double z = 0.0);
	void append_vertex(const Vector3d &v);
	void append_vertex(const Vector3f &v);
	void insert_vertex(double x, double y, double z = 0.0);
	void insert_vertex(const Vector3d &v);
	void insert_vertex(const Vector3f &v);
	void append(const PolySet &ps);

	void render_surface(Renderer::csgmode_e csgmode, const Transform3d &m, GLint *shaderinfo = nullptr) const;
	void render_edges(Renderer::csgmode_e csgmode) const;

	void transform(const Transform3d &mat);
	void resize(const Vector3d &newsize, const Eigen::Matrix<bool,3,1> &autosize);

	bool is_convex() const;
	boost::tribool convexValue() const { return this->convex; }

private:
	Polygon2d polygon;
	unsigned int dim;
	mutable boost::tribool convex;
	mutable BoundingBox bbox;
	mutable bool dirty;
};
