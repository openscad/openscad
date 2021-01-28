#pragma once

#include "polyset.h"
#include "node.h"
#include "CGAL_Nef_polyhedron.h"
#include "linalg.h"

#include "cgal.h"
#include "cgalutils.h"

/*! Holds a Geometry and allows defered conversion to Nef or PolySet. Also,
 * looks up the cache using the provided keys.
 *
 * TODO(ochafik): Find a way to unite GeometryEvaluator's smart cache with this.
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

	LazyGeometry() : geom(nullptr), pNode(nullptr) {}
	LazyGeometry(const geom_ptr_t &geom, const AbstractNode *pNode = nullptr)
		: geom(geom), pNode(pNode)
	{
	}
	LazyGeometry(const LazyGeometry &other)
		: geom(other.geom), pNode(other.pNode), polyhedron(other.polyhedron)
	{
	}

	operator bool() const { return getGeom().get() != nullptr; }
	geom_ptr_t getGeom() const;
	bool isPolySet() const;
	bool isNef() const;
	CGAL_Iso_cuboid_3 getBoundingBox() const;
	LazyGeometry concatenateDisjoint(const LazyGeometry &other,
																	 const get_cache_key_fn_t &get_cache_key) const;
	LazyGeometry joinProbablyOverlapping(const LazyGeometry &other,
																			 const get_cache_key_fn_t &get_cache_key) const;
	nef_ptr_t getNef(const get_cache_key_fn_t &get_cache_key) const;
	polyset_ptr_t getPolySet(const get_cache_key_fn_t &get_cache_key) const;

private:
	polyset_ptr_t concatenateDisjointPolySets(const PolySet &a, const PolySet &b) const;
	polyhedron_ptr_t getPolyhedron_onlyIfGeomIsNef() const;

	geom_ptr_t geom;
	// The node this geometry corresponds to, if any
	// (nullptr for intermediate operations; top level operations will be cached
	// in GeometryEvaluator).
	const AbstractNode *pNode;
	mutable polyhedron_ptr_t polyhedron;
};
