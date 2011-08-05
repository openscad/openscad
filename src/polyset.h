#ifndef POLYSET_H_
#define POLYSET_H_

#include <GL/glew.h>
#include "grid.h"
#include <vector>
#include <Eigen/Core>
#include <Eigen/Geometry>

using Eigen::Vector3d;
typedef Eigen::AlignedBox<double, 3> BoundingBox;

class PolySet
{
public:
	typedef std::vector<Vector3d> Polygon;
	std::vector<Polygon> polygons;
	std::vector<Polygon> borders;
	Grid3d<void*> grid;

	bool is2d;
	int convexity;

	PolySet();
	~PolySet();

	void append_poly();
	void append_vertex(double x, double y, double z = 0.0);
	void insert_vertex(double x, double y, double z = 0.0);

	BoundingBox getBoundingBox() const;

	enum colormode_e {
		COLORMODE_NONE,
		COLORMODE_MATERIAL,
		COLORMODE_CUTOUT,
		COLORMODE_HIGHLIGHT,
		COLORMODE_BACKGROUND
	};

	enum csgmode_e {
		CSGMODE_NONE,
		CSGMODE_NORMAL = 1,
		CSGMODE_DIFFERENCE = 2,
		CSGMODE_BACKGROUND = 11,
		CSGMODE_BACKGROUND_DIFFERENCE = 12,
		CSGMODE_HIGHLIGHT = 21,
		CSGMODE_HIGHLIGHT_DIFFERENCE = 22
	};

	void render_surface(colormode_e colormode, csgmode_e csgmode, double *m, GLint *shaderinfo = NULL) const;
	void render_edges(colormode_e colormode, csgmode_e csgmode) const;

	int refcount;
	PolySet *link();
	void unlink();
};

#endif
