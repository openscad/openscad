// Portions of this file are Copyright 2021 Google LLC, and licensed under GPL2+. See COPYING.

#pragma once

#include "GeometryCache.h"
#include "CGALCache.h"
#include "polyset.h"
#include "CGAL_Nef_polyhedron.h"
#include "Tree.h"
#include "cgalutils.h"
#include "ModuleInstantiation.h"
#include "scoped_timer.h"

inline std::string getCacheKey(const AbstractNode *node, const Tree *tree)
{
	return tree && node ? tree->getIdString(*node) : "";
}

inline Location getLocation(const AbstractNode *node)
{
	return node && node->modinst ? node->modinst->location() : Location::NONE;
}

/*! Cache helper that respects the caller's type wishes.
 * TODO(ochafik): Phase out GeometryEvaluator::smartCache* using this.
 */
class MixedCache
{
public:
	template <typename T>
	static shared_ptr<const T> getOrSet(const std::string &key,
																			const std::function<shared_ptr<const T>()> &f,
																			bool lookupConvertibleGeometries = false)
	{
		assert(!lookupConvertibleGeometries && "TODO!");

		if (key != "") {
			if (std::is_assignable<CGAL_Nef_polyhedron, T>::value &&
					CGALCache::instance()->contains(key)) {
				auto value = CGALCache::instance()->get(key);
				auto cast = dynamic_pointer_cast<const T>(value);
				if ((value == nullptr) == (cast == nullptr)) return cast;
			}
			else if (GeometryCache::instance()->contains(key)) {
				auto value = GeometryCache::instance()->get(key);
				auto cast = dynamic_pointer_cast<const T>(value);
				if ((value == nullptr) == (cast == nullptr)) return cast;
			}
		}

		auto result = f();
		if (key != "") {
			if (auto nef = dynamic_pointer_cast<const CGAL_Nef_polyhedron>(result)) {
				CGALCache::instance()->insert(key, nef);
			}
			else if (auto polyset = dynamic_pointer_cast<const PolySet>(result)) {
				GeometryCache::instance()->insert(key, polyset);
			}
		}
		return result;
	}
};

/*! Gets the geometry in the requested format. If the geometry isn't in the
 * correct format already, looks up the conversion in the cache and converts
 * it + caches it if it wasn't there yet.
 */
template <typename T>
shared_ptr<const T> getGeometryAs(const shared_ptr<const Geometry> &geom,
																	const AbstractNode *node = nullptr, const Tree *tree = nullptr)
{
	if (!geom) return nullptr;

	shared_ptr<const Geometry> result;
	if (std::is_assignable<CGAL_Nef_polyhedron, T>::value) {
		if (auto nef = dynamic_pointer_cast<const CGAL_Nef_polyhedron>(geom)) {
			result = nef;
		}
		else if (auto polyset = dynamic_pointer_cast<const PolySet>(geom)) {
			result = MixedCache::getOrSet<CGAL_Nef_polyhedron>(getCacheKey(node, tree), [&]() {
				SCOPED_PERFORMANCE_TIMER("PolySet -> Nef conversion in MixedCache::getAs");
				return shared_ptr<const CGAL_Nef_polyhedron>(
						CGALUtils::createNefPolyhedronFromGeometry(*polyset));
			});
		}
		else {
			assert(!"Unsupported conversion to nef");
		}
	}
	else if (std::is_assignable<PolySet, T>::value) {
		if (auto polyset = dynamic_pointer_cast<const PolySet>(geom)) {
			result = polyset;
		}
		else if (auto nef = dynamic_pointer_cast<const CGAL_Nef_polyhedron>(geom)) {
			result = MixedCache::getOrSet<PolySet>(getCacheKey(node, tree), [&]() {
				SCOPED_PERFORMANCE_TIMER("Nef -> PolySet conversion in MixedCache::getAs");
				auto polyset = make_shared<PolySet>(3);
				bool err = CGALUtils::createPolySetFromNefPolyhedron3(*nef->p3, *polyset);
				if (err) {
					LOG(message_group::Error, getLocation(node), "",
							"LazyGeometry::getPolySet: Nef->PolySet failed");
				}
				return polyset;
			});
		}
		else {
			assert(!"Unsupported conversion to polyset");
		}
	}

	// A specialization for each T would have been nicer, but couldn't work out
	// linker issues despite marking the specialization as inline.
	auto cast_result = dynamic_pointer_cast<const T>(result);
	assert((result == nullptr) == (cast_result == nullptr));

	return cast_result;
}

template <typename T>
shared_ptr<const T> getGeometryAs(const Geometry::GeometryItem &item, const Tree *tree)
{
	return getGeometryAs<T>(item.second, item.first, tree);
}
