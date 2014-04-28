#pragma once

#include "Geometry.h"
#include "system-gl.h"
#include "linalg.h"
#include "renderer.h"
#include "Polygon2d.h"
#include <vector>
#include <string>

class PolySet : public Geometry
{
public:
	typedef std::vector<Vector3d> Polygon;
	std::vector<Polygon> polygons;

	PolySet(unsigned int dim);
	PolySet(const Polygon2d &origin);
	virtual ~PolySet();

	virtual size_t memsize() const;
	virtual BoundingBox getBoundingBox() const;
	virtual std::string dump() const;
	virtual unsigned int getDimension() const { return this->dim; }
	virtual bool isEmpty() const { return polygons.size() == 0; }

	size_t numPolygons() const { return polygons.size(); }
	void append_poly();
	void append_vertex(double x, double y, double z = 0.0);
	void append_vertex(Vector3d v);
	void insert_vertex(double x, double y, double z = 0.0);
	void insert_vertex(Vector3d v);
	void append(const PolySet &ps);

	void render_surface(Renderer::csgmode_e csgmode, const Transform3d &m, GLint *shaderinfo = NULL) const;
	void render_edges(Renderer::csgmode_e csgmode) const;

	void transform(const Transform3d &mat);
	void resize(Vector3d newsize, const Eigen::Matrix<bool,3,1> &autosize);

private:
  Polygon2d polygon;
	unsigned int dim;
};
