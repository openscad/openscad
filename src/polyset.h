#ifndef POLYSET_H_
#define POLYSET_H_

#include "system-gl.h"
#include "grid.h"
#include "linalg.h"
#include <vector>
#include <string>

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

	bool empty() const { return polygons.size() == 0; }
	void append_poly();
	void append_vertex(double x, double y, double z = 0.0);
	void insert_vertex(double x, double y, double z = 0.0);
	size_t memsize() const;

	BoundingBox getBoundingBox() const;

#define CSGMODE_DIFFERENCE_FLAG 0x10
	enum csgmode_e {
		CSGMODE_NONE                  = 0x00,
		CSGMODE_NORMAL                = 0x01,
		CSGMODE_DIFFERENCE            = CSGMODE_NORMAL | CSGMODE_DIFFERENCE_FLAG,
		CSGMODE_BACKGROUND            = 0x02,
		CSGMODE_BACKGROUND_DIFFERENCE = CSGMODE_BACKGROUND | CSGMODE_DIFFERENCE_FLAG,
		CSGMODE_HIGHLIGHT             = 0x03,
		CSGMODE_HIGHLIGHT_DIFFERENCE  = CSGMODE_HIGHLIGHT | CSGMODE_DIFFERENCE_FLAG
	};

	void render_surface(csgmode_e csgmode, const Transform3d &m, GLint *shaderinfo = NULL) const;
	void render_edges(csgmode_e csgmode) const;
	std::string dump() const;
};

#endif
