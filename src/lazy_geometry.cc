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

LazyGeometry::LazyGeometry() : pNode(nullptr) {}

LazyGeometry::LazyGeometry(const geom_ptr_t &geom, const AbstractNode *pNode)
	: geom(geom), pNode(pNode)
{
	assert(geom);
	assert(isPolySet() || isNef());
}

LazyGeometry::LazyGeometry(const LazyGeometry &other) : geom(other.geom), pNode(other.pNode)
{
	assert(geom);
	assert(isPolySet() || isNef());
}

LazyGeometry &LazyGeometry::operator=(const LazyGeometry &other)
{
	geom = other.geom;
	pNode = other.pNode;
	assert(geom);
	assert(isPolySet() || isNef());
	return *this;
}

LazyGeometry::get_cache_key_fn_t LazyGeometry::no_get_cache_key_fn = [](const AbstractNode &node) {
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

BoundingBoxes::BoundingBoxoid LazyGeometry::getBoundingBox() const
{
	if (auto polyset = dynamic_pointer_cast<const PolySet>(geom)) {
		return polyset->getBoundingBox();
	}
	else {
		auto nef = dynamic_pointer_cast<const CGAL_Nef_polyhedron>(geom);
		assert(nef);
		auto res = CGALUtils::boundingBox(*nef->p3);
		return res;
	}
}

LazyGeometry LazyGeometry::concatenateDisjoint(const LazyGeometry &other,
																							 const get_cache_key_fn_t &get_cache_key) const
{
	// No matter what, we don't care if we have nefs here.
	// The assumption is that joining two nefs is always more costly than creating
	// the nef resulting from joining the polysets (also, it's not even sure we'd
	// need a Nef result), but that might not be true in all cases.

	PolySet other_copy(*other.getPolySet(get_cache_key));
	other_copy.quantizeVertices();

	auto concatenation = shared_ptr<PolySet>(new PolySet(*getPolySet(get_cache_key)));
	concatenation->quantizeVertices();
	concatenation->append(other_copy);
	concatenation->quantizeVertices();

	CGAL_Polyhedron P;
	auto err = CGALUtils::createPolyhedronFromPolySet(*concatenation, P);
	if (err || !P.is_closed() || !P.is_valid(false, 0)) {
		LOG(message_group::Warning, Location::NONE, "",
				"Fast union result couldn't be converted to a polyhedron. Reverting to slower full union.");
		return joinProbablyOverlapping(other, get_cache_key);
	}

	return LazyGeometry(concatenation);
}

LazyGeometry LazyGeometry::joinProbablyOverlapping(
	const LazyGeometry &other, const get_cache_key_fn_t &get_cache_key) const
{
	auto nef1 = getNef(get_cache_key);
	auto nef2 = other.getNef(get_cache_key);
	if (!nef2->p3) return LazyGeometry(nef1);
	if (!nef1->p3) return LazyGeometry(nef2);

	return LazyGeometry(nef_ptr_t(new CGAL_Nef_polyhedron(*nef1 + *nef2)));
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
	auto ps = new PolySet(3);
	auto converted = LazyGeometry::polyset_ptr_t(ps);

	auto nef = getNef(get_cache_key);
	assert(nef);
	bool err = CGALUtils::createPolySetFromNefPolyhedron3(*nef->p3, *ps);
	if (err) {
		LOG(message_group::Error, Location::NONE, "", "LazyGeometry::getPolySet: Nef->PolySet failed");
		return converted;
	}
	if (pNode && key != "") {
		GeometryCache::instance()->insert(key, converted);
	}
	return converted;
}
