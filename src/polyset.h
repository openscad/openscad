#ifndef POLYSET_H_
#define POLYSET_H_

#ifdef ENABLE_OPENCSG
// this must be included before the GL headers
#  include <GL/glew.h>
#endif
#include <qgl.h>

#include "grid.h"
#ifdef ENABLE_OPENCSG
#  include <opencsg.h>
#endif
#ifdef ENABLE_CGAL
#  include "cgal.h"
#endif

#include <QCache>

class PolySet
{
public:
	struct Point {
		double x, y, z;
		Point() : x(0), y(0), z(0) { }
		Point(double x, double y, double z) : x(x), y(y), z(z) { }
	};
	typedef QList<Point> Polygon;
	QVector<Polygon> polygons;
	QVector<Polygon> borders;
	Grid3d<void*> grid;

	bool is2d;
	int convexity;

	PolySet();
	~PolySet();

	void append_poly();
	void append_vertex(double x, double y, double z);
	void insert_vertex(double x, double y, double z);

	void append_vertex(double x, double y) {
		append_vertex(x, y, 0.0);
	}
	void insert_vertex(double x, double y) {
		insert_vertex(x, y, 0.0);
	}

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

	struct ps_cache_entry {
		PolySet *ps;
		QString msg;
		ps_cache_entry(PolySet *ps);
		~ps_cache_entry();
	};

	static QCache<QString,ps_cache_entry> ps_cache;

	void render_surface(colormode_e colormode, csgmode_e csgmode, double *m, GLint *shaderinfo = NULL) const;
	void render_edges(colormode_e colormode, csgmode_e csgmode) const;

#ifdef ENABLE_CGAL
	CGAL_Nef_polyhedron renderCSGMesh() const;
#endif

	int refcount;
	PolySet *link();
	void unlink();
};

#endif
