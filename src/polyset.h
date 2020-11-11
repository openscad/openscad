#pragma once

#include <unordered_map>
#include <functional>
#include <vector>
#include <algorithm>
#include "hash.h"

#include "Geometry.h"
#include "linalg.h"
#include "GeometryUtils.h"
#include "Polygon2d.h"
#include "GLView.h"
#include <string>

#include <boost/logic/tribool.hpp>
BOOST_TRIBOOL_THIRD_STATE(unknown)

typedef std::unordered_map<Vector3d,size_t> VertexIndexMap;

namespace /* anonymous */ {
        template<typename Result, typename V, typename T = double>
	Result polygon_convert(V const& v) {
                return Result(T(v[0]),T(v[1]),T(v[2]));
       	}
}

namespace /* anonymous */ {
        template<typename Result, typename V, typename T = double>
	Result array_convert(V const& v) {
                return Result({T(v[0]),T(v[1]),T(v[2])});
       	}
}

class PolySet : public Geometry
{
public:
	VISITABLE_GEOMETRY();

	PolySet();
	PolySet(unsigned int dim, boost::tribool convex = unknown);
	PolySet(const Polygon2d &origin);
	~PolySet();

	const Polygon2d &getPolygon() const { return this->polygon; }
	const Polygons &getPolygons() const { return this->polygons; }
	const IndexedPolygons &getIndexedPolygons() const { return this->indexed_polygons; }
	const IndexedTriangles &getIndexedTriangles() const { const_cast<PolySet *>(this)->tessellate(); return this->indexed_triangles; }
	template<typename T, class C = std::vector<T>>
	void getVertices(C &vertices, std::function<T(Vector3d const &)> convert = polygon_convert<T, Vector3d>) const {
		vertices.resize(this->vi_map.size());
		for (const auto &v : this->vi_map) {
			typename C::iterator el = vertices.begin() + v.second;
			(*el) = convert(v.first);
		}
	}

	size_t memsize() const override;
	BoundingBox getBoundingBox() const override;
	std::string dump() const override;
	unsigned int getDimension() const override { return this->dim; }
	bool isEmpty() const override { return polygons.size() == 0; }
	bool isConvex() const;
	bool isQuantized() const { return (this->quantized > 0); }

	Geometry *copy() const override { return new PolySet(*this); }
	void copyPolygons(const PolySet &ps);

	size_t numFacets() const override { return this->num_facets; }
	size_t numDegeneratePolygons() const { return this->num_degenerate_polygons; }
	void append_poly();
	void append_poly_only();
	void append_poly(const Polygon &poly);
	void append_facet();
	void append_vertex(double x, double y, double z = 0.0);
	void append_vertex(const Vector3d &v);
	void append_vertex(const Vector3f &v);
	void insert_vertex(double x, double y, double z = 0.0);
	void insert_vertex(const Vector3d &v);
	void insert_vertex(const Vector3f &v);
	void append(const PolySet &ps);
	void close_facet();
	void close_poly();
	void close_poly_only();

	void transform(const Transform3d &mat);
	void translate(const Vector3d &translation);
	void reverse();

	void resize(const Vector3d &newsize, const Eigen::Matrix<bool,3,1> &autosize);
	boost::tribool convexValue() const { return this->convex; }

	void reindex();
	void quantizeVertices();
	void tessellate();
	Polygon2d *project() const;

private:
	size_t vertexLookup(const Vector3d &vertex);

private:
	Polygons polygons;
	Polygon2d polygon;

	mutable VertexIndexMap vi_map;
	mutable IndexedPolygons indexed_polygons;
	mutable IndexedTriangles indexed_triangles;
	mutable size_t num_facets;
	mutable size_t num_degenerate_polygons;
	size_t num_appends;

	unsigned int dim;
	mutable boost::tribool convex;
	mutable BoundingBox bbox;
	mutable bool dirty;
	mutable bool triangles_dirty;
	size_t quantized;
	size_t reversed;
};
