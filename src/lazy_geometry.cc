#include "GeometryCache.h"
#include "CGALCache.h"
#include "polyset.h"
#include "node.h"
#include "CGAL_Nef_polyhedron.h"
#include "linalg.h"

#include "cgal.h"
#include "cgalutils.h"

#include "bounding_boxes.h"
#include "lazy_geometry.h"

LazyGeometry::get_cache_key_fn_t LazyGeometry::no_get_cache_key_fn = [](const AbstractNode& node) {
  return "";
};

LazyGeometry::geom_ptr_t LazyGeometry::getGeom() const
{
	return geom;
}
bool LazyGeometry::isPolySet() const
{
	return dynamic_pointer_cast<const PolySet>(geom).get() != nullptr;
}
bool LazyGeometry::isNef() const
{
	return dynamic_pointer_cast<const CGAL_Nef_polyhedron>(geom).get() != nullptr;
}

CGAL_Iso_cuboid_3 LazyGeometry::getBoundingBox() const
{
	if (auto polyset = dynamic_pointer_cast<const PolySet>(geom)) {
		return BoundingBoxes::bboxToIsoCuboid(polyset->getBoundingBox());
	}
	else {
		auto poly = getPolyhedron_onlyIfGeomIsNef();
		return CGAL::bounding_box(poly->points_begin(), poly->points_end());
	}
}
// BoundingBox LazyGeometry::getBoundingBox() const {
//   if (auto polyset = dynamic_pointer_cast<const PolySet>(geom)) {
//     return polyset->getBoundingBox();
//   } else {
//     auto poly = getPolyhedron_onlyIfGeomIsNef();
//     auto isoCuboid = CGAL::bounding_box(poly->points_begin(), poly->points_end());
//     return BoundingBoxes::isoCuboidToBbox(isoCuboid);
//   }
// }

LazyGeometry LazyGeometry::concatenateDisjoint(const LazyGeometry &other,
																							 const get_cache_key_fn_t &get_cache_key) const
{
	// No matter what, we don't care if we have nefs here.
	// The assumption is that joining two nefs is always more costly than creating one, but that might
	// not be true.
	// TODO(ochafik): Try and concatenate nefs?
	return LazyGeometry(
			concatenateDisjointPolySets(*getPolySet(get_cache_key), *other.getPolySet(get_cache_key)));
}

LazyGeometry LazyGeometry::joinProbablyOverlapping(const LazyGeometry &other,
																									 const get_cache_key_fn_t &get_cache_key) const
{
	return LazyGeometry(LazyGeometry::nef_ptr_t(
			new CGAL_Nef_polyhedron(*getNef(get_cache_key) + *other.getNef(get_cache_key))));
}

LazyGeometry::nef_ptr_t LazyGeometry::getNef(const get_cache_key_fn_t &get_cache_key) const
{
	if (auto nef = dynamic_pointer_cast<const CGAL_Nef_polyhedron>(geom)) {
		return nef;
	}
	auto key = pNode ? get_cache_key(*pNode) : "";
	if (pNode && key != "" && CGALCache::instance()->contains(key)) {
		auto cached = CGALCache::instance()->get(key);
		if (cached) return cached;
	}

	auto converted = LazyGeometry::nef_ptr_t(CGALUtils::createNefPolyhedronFromGeometry(*geom));
	if (pNode && key != "") {
		CGALCache::instance()->insert(key, converted);
	}
	return converted;
}

LazyGeometry::polyset_ptr_t LazyGeometry::getPolySet(const get_cache_key_fn_t &get_cache_key) const
{
	if (auto polyset = dynamic_pointer_cast<const PolySet>(geom)) {
		return polyset;
	}
	auto key = pNode ? get_cache_key(*pNode) : "";
	if (pNode && key != "" && GeometryCache::instance()->contains(key)) {
		auto cached = dynamic_pointer_cast<const PolySet>(GeometryCache::instance()->get(key));
		if (cached) return cached;
	}

	// ScopedTimer timer("CGALUtils::applyUnion3D -> polyhedronToPolySet");
	auto poly = getPolyhedron_onlyIfGeomIsNef();
	auto ps = new PolySet(3);
	CGALUtils::createPolySetFromPolyhedron(*poly, *ps);
	auto converted = LazyGeometry::polyset_ptr_t(ps);
	if (pNode && key != "") {
		GeometryCache::instance()->insert(key, converted);
	}
	return converted;
}

LazyGeometry::polyset_ptr_t LazyGeometry::concatenateDisjointPolySets(const PolySet &a,
																																			const PolySet &b) const
{
	// ScopedTimer timer("CGALUtils::applyUnion3D -> polyConcat");
	auto c = new PolySet(a);
	c->append(b);
	return LazyGeometry::polyset_ptr_t(c);
}

LazyGeometry::polyhedron_ptr_t LazyGeometry::getPolyhedron_onlyIfGeomIsNef() const
{
	if (!polyhedron) {
		auto nef = dynamic_pointer_cast<const CGAL_Nef_polyhedron>(geom);
		assert(nef);
		auto poly = new CGAL_Polyhedron();
		nef->p3->convert_to_polyhedron(*poly);
		polyhedron = LazyGeometry::polyhedron_ptr_t(poly);
	}
	return polyhedron;
}
