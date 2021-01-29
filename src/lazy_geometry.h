#pragma once

#include "polyset.h"
#include "node.h"
#include "CGAL_Nef_polyhedron.h"
#include "linalg.h"

#include "bounding_boxes.h"
#include "cgal.h"
#include "cgalutils.h"

/*! Holds a Geometry and allows deferred conversion to Nef or PolySet. Also,
 * looks up the cache using the provided keys.
 */
class LazyGeometry
{
public:
	typedef shared_ptr<const Geometry> geom_ptr_t;
	typedef shared_ptr<const CGAL_Nef_polyhedron> nef_ptr_t;
	typedef shared_ptr<const PolySet> polyset_ptr_t;
	typedef shared_ptr<const CGAL_Polyhedron> polyhedron_ptr_t;
	typedef std::function<std::string(const AbstractNode &node)> get_cache_key_fn_t;
	static get_cache_key_fn_t no_get_cache_key_fn;

	LazyGeometry();
	LazyGeometry(const geom_ptr_t &geom, const AbstractNode *pNode = nullptr);
	LazyGeometry(const LazyGeometry &other);

	LazyGeometry &operator=(const LazyGeometry &other);

	operator bool() const { return getGeom().get() != nullptr; }
	geom_ptr_t getGeom() const;
	bool isPolySet() const;
	bool isNef() const;
	BoundingBoxes::BoundingBoxoid getBoundingBox() const;

  /*! Does a union assuming this geometry and other are physically disjoint,
   * i.e. their polygons don't intersect and none of them is containing the
   * other. */
	LazyGeometry concatenateDisjoint(const LazyGeometry &other,
																	 const get_cache_key_fn_t &get_cache_key) const;

  /*! Does a union assuming the two geometries are likely to intersect. */
	LazyGeometry joinProbablyOverlapping(const LazyGeometry &other,
																			 const get_cache_key_fn_t &get_cache_key) const;
	nef_ptr_t getNef(const get_cache_key_fn_t &get_cache_key) const;
	polyset_ptr_t getPolySet(const get_cache_key_fn_t &get_cache_key) const;

private:
	geom_ptr_t geom;
	// The node this geometry corresponds to, if any
	// (nullptr for intermediate operations; top level operations will be cached
	// in GeometryEvaluator). This is used to compute the caching key used to
  // fetch / store nefs converted to polysets and vice versa.
	const AbstractNode *pNode;
};
