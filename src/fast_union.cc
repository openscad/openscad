// Portions of this file are Copyright 2021 Google LLC, and licensed under GPL2+. See COPYING.

#include <list>
#include <unordered_map>
#include "mixed_cache.h"
#include "polyset.h"
#include "node.h"
#include "hash.h"

#include "CGAL_Nef_polyhedron.h"
#include "linalg.h"

#include "cgal.h"
#include "cgalutils.h"

#include "fast_union.h"

bool reduceFastUnionableGeometries(Geometry::Geometries &children, const Tree* tree)
{
  auto analysis = analyzeUnion(children.begin(), children.end(), tree);

  std::vector<Geometry::GeometryItem> remainingGeometries;
  remainingGeometries.insert(remainingGeometries.end(), analysis.otherGeometries.begin(), analysis.otherGeometries.end());

  if (analysis.nonIntersectingGeometries.empty()) return false;

  for (auto &cluster : analysis.nonIntersectingGeometries) {
    assert(!cluster.empty());
    auto fastConcat = unionNonIntersecting(cluster, tree);
    if (fastConcat) {
      remainingGeometries.emplace_back(std::make_pair(nullptr, fastConcat));
    } else {
      remainingGeometries.insert(remainingGeometries.end(), cluster.begin(), cluster.end());
      LOG(message_group::Warning, getLocation(cluster.begin()->first),
          "", "fast-union of %1$lu solids failed, doing slower nef unions", cluster.size());
    }
  }

  children.clear();
  children.insert(children.end(), remainingGeometries.begin(), remainingGeometries.end());
  return true;
}

// This bugdet controls the overlap detection logic.
// We do up to this amound of unsuccessful tests to find "easy" unions for each
// geometry being unioned. Since we don't end up testing all the other
// geometries, it's not a quadratic search (even if the constant is big). But
// that's not to say it's linear either.
#define FAST_JOIN_EXPLORATION_BUDGET 100

/*! Analyze the operands of a union to spot opportunities for faster subunions
 * of clusters of mutually non-intersecting geometries.
 * CGAL_Nef_polyhedron are costly to create and their unions are slow, even if
 * the geometries of the unioned solids are disjoint, so this helps save a lot
 * of render time, up to 2 orders of magnitude for some models (especially
 * grid-like patterns).
 */
UnionAnalysis analyzeUnion(Geometry::Geometries::iterator chbegin,
													 Geometry::Geometries::iterator chend, const Tree *tree)
{
	// https://github.com/CGAL/cgal/blob/master/Union_find/include/CGAL/Union_find.h
	typedef CGAL::Union_find<Geometry::GeometryItem> Clusters;
	typedef Clusters::handle Cluster;

	Clusters clusters;

	// TODO(ochafik): Try and use CGAL::Box_intersection_d instead of these crude
  // lists of bboxes (An issue is Box_intersection_d finds intersecting pairs,
  // need to find a way to use that information to build non-intersecting
  // clusters in less than quadratic time).
	std::unordered_map<Geometry::GeometryItem, std::vector<CGAL_Iso_cuboid_3>> clusterBboxes;

	// https://doc.cgal.org/latest/Spatial_sorting/index.html
  std::vector<Geometry::GeometryItem> sortedGeometries;
	sortedGeometries.insert(sortedGeometries.end(), chbegin, chend);
	CGALUtils::spatialSort(sortedGeometries);

	std::vector<std::pair<Geometry::GeometryItem, Cluster>> initialHandles;
	for (auto &item : sortedGeometries) {
		auto geom = item.second;
		if (!geom) {
			LOG(message_group::Warning, Location::NONE, "", "Missing geom in union analysis");
			continue;
		}
		auto cluster = clusters.make_set(item);
		initialHandles.emplace_back(item, cluster);
		clusterBboxes[item].push_back(CGALUtils::boundingBox(geom));
	}

	// Use an intermediate list because we know its iterators are stable.
	// The list represents the same as the Union_find, though, except with
	// just their handle = ptr to the geometry.
	std::list<Cluster> clusterHandles;
	for (auto handle = clusters.begin(); handle != clusters.end(); handle++) {
		clusterHandles.push_back(handle);
	}
	// std::for_each(clusters.begin(), clusters.end(), std::back_inserter(clusterHandles));

	for (auto it1 = clusterHandles.begin(); it1 != clusterHandles.end(); it1++) {
		auto it2 = it1;
		it2++;

		auto remaining_budget = FAST_JOIN_EXPLORATION_BUDGET;
		while (it2 != clusterHandles.end() && remaining_budget > 0) {
			auto cluster1 = *it1;
			auto cluster2 = *it2;
			auto bboxes1 = clusterBboxes[*cluster1];
			auto bboxes2 = clusterBboxes[*cluster2];
			assert(bboxes1.size());
			assert(bboxes2.size());

			auto clustersAreDisjoint = ([&]() {
				for (auto &b1 : bboxes1) {
					for (auto &b2 : bboxes2) {
						if (CGAL::intersection(b1, b2) != boost::none) {
							return false;
						}
					}
				}
				return true;
			})();

			if (clustersAreDisjoint) {
				// Found an easy union!
				clusters.unify_sets(cluster1, cluster2);
				auto new_cluster = clusters.find(cluster1);
				auto &bboxesDestination = clusterBboxes[*new_cluster];
				auto &bboxesToCopy = clusterBboxes[cluster1 == new_cluster ? *cluster2 : *cluster1];
				bboxesDestination.insert(bboxesDestination.end(), bboxesToCopy.begin(), bboxesToCopy.end());

				*it1 = new_cluster;
				// *it1 = make_shared<const LazyGeometry>(**it1 + **it2);
				it2 = clusterHandles.erase(it2);
				remaining_budget = FAST_JOIN_EXPLORATION_BUDGET;
			}
			else {
				remaining_budget--;
				it2++;
			}
		}

		if (remaining_budget == 0) {
			LOG(message_group::Echo, Location::NONE, "",
					"Exhausted search budget for fast-union pairs finding");
		}
	}

	// We have a union find structure: use it to build the physical vectors of clustered values.
	std::unordered_map<Geometry::GeometryItem, std::vector<Geometry::GeometryItem>> clustersAsVectors;

	for (auto &initialHandle : initialHandles) {
		auto &geom = initialHandle.first;
		auto cluster = clusters.find(initialHandle.second);
		auto &vector = clustersAsVectors[*cluster];
		vector.push_back(geom);
	}

	// We'll now split clusters that will be fast-unionable from the rest
	UnionAnalysis out;
	out.nonIntersectingGeometries.reserve(clustersAsVectors.size());

	for (auto it = clustersAsVectors.begin(); it != clustersAsVectors.end(); it++) {
		auto &vector = it->second;
		if (vector.size() > 1) {
			size_t total = 0;
			for (auto &g : vector) total += CGALUtils::getNumberOfFacets(g.second);
			LOG(message_group::Echo, getLocation(it->first.first), "",
					"Found set of %1$lu disjoint geometries with %2$lu total facets.", vector.size(),
					total);
			out.nonIntersectingGeometries.emplace_back();
			auto &list = out.nonIntersectingGeometries.back();
			// Really we should make Geometry::Geometries a vector?
			list.insert(list.end(), vector.begin(), vector.end());
		}
		else {
			assert(vector.size() == 1);
			out.otherGeometries.push_back(vector[0]);
		}
	}

	return out;
}

shared_ptr<const PolySet> unionNonIntersecting(const Geometry::Geometries &geometries,
																							 const Tree *tree)
{
	size_t totalFacets = 0;
	std::vector<shared_ptr<const PolySet>> polysets;
	for (auto &item : geometries) {
		if (auto polyset = getGeometryAs<PolySet>(item, tree)) {
			polysets.push_back(polyset);
			totalFacets += polyset->numFacets();
		}
		else {
			LOG(message_group::Error, Location::NONE, "", "Skipping null polyset in fast union");
		}
	}

	auto concatenation = shared_ptr<PolySet>(new PolySet(3));
	concatenation->reserve(totalFacets);
	for (auto &polyset : polysets) {
		concatenation->append(*polyset);
	}
	concatenation->quantizeVertices();

#ifndef OPTIMISTIC_FAST_UNION
	{

		// Very costly detection of pathological cases (can represent more than half
		// the render time for purely-fast-unioned assemblies).
		CGAL_Polyhedron P;
		auto err = CGALUtils::createPolyhedronFromPolySet(*concatenation, P);
		if (err || !P.is_closed() || !P.is_valid(false, 0)) {
			LOG(message_group::Warning, Location::NONE, "",
					"Fast union result couldn't be converted to a polyhedron. Reverting to slower full "
					"union.");
			return nullptr;
		}
	}
#endif

	return concatenation;
}
