#include "CGALNefEvaluator.h"
#include "Tree.h"
#include "GeometryCache.h"
#include "Polygon2d.h"
#include "module.h"
#include "ModuleInstantiation.h"
#include "state.h"
#include "transformnode.h"
#include "csgnode.h"
#include "projectionnode.h"
#include "csgops.h"
#include "rendernode.h"
#include "clipper-utils.h"
#include "polyset-utils.h"
#include "polyset.h"
#include "calc.h"
#include "printutils.h"
#include "svg.h"
#include "calc.h"

#include "cgalutils.h"
#include "CGAL_Nef_polyhedron.h"
#include "CGALCache.h"
#include "cgaladvnode.h"

#include <ciso646> // C alternative tokens (xor)
#include <algorithm>


CGALNefEvaluator::CGALNefEvaluator(const class Tree &tree):
	GeometryEvaluator(tree)
{
}

/*!
	Set allownef to false to force the result to _not_ be a Nef polyhedron
*/
shared_ptr<const Geometry> CGALNefEvaluator::evaluateGeometry(const AbstractNode &node, 
																															 bool allownef)
{
	const std::string &key = this->tree.getIdString(node);
	if (!GeometryCache::instance()->contains(key)) {
		shared_ptr<const CGAL_Nef_polyhedron> N;
		if (CGALCache::instance()->contains(key)) {
			N = CGALCache::instance()->get(key);
		}

		// If not found in any caches, we need to evaluate the geometry
		if (N) {
			this->root = N;
		}	
    else {
			this->traverse(node);
		}

		if (!allownef) {
			if (shared_ptr<const CGAL_Nef_polyhedron> N = dynamic_pointer_cast<const CGAL_Nef_polyhedron>(this->root)) {
				PolySet *ps = new PolySet(3);
				ps->setConvexity(N->getConvexity());
				this->root.reset(ps);
				if (!N->isEmpty()) {
					bool err = CGALUtils::createPolySetFromNefPolyhedron3(*N->p3, *ps);
					if (err) {
						PRINT("ERROR: Nef->PolySet failed");
					}
				}
			}
		}
		smartCacheInsert(node, this->root);
		return this->root;

	}
	return GeometryCache::instance()->get(key);
}

/*!
	Applies the operator to all child nodes of the given node.
	
	May return nullptr or any 3D Geometry object (can be either PolySet or CGAL_Nef_polyhedron)
*/
GeometryEvaluator::ResultObject CGALNefEvaluator::applyToChildren3D(const AbstractNode &node, OpenSCADOperator op)
{
	Geometry::Geometries children = collectChildren3D(node);
	if (children.size() == 0) return ResultObject();

	if (op == OpenSCADOperator::HULL) {
		PolySet *ps = new PolySet(3, true);

		if (CGALUtils::applyHull(children, *ps)) {
			return ps;
		}

		delete ps;
		return ResultObject();
	}
	
	// Only one child -> this is a noop
	if (children.size() == 1) return ResultObject(children.front().second);

	if (op == OpenSCADOperator::MINKOWSKI) {
		Geometry::Geometries actualchildren;
		for(const auto &item : children) {
			if (!item.second->isEmpty()) actualchildren.push_back(item);
		}
		if (actualchildren.empty()) return ResultObject();
		if (actualchildren.size() == 1) return ResultObject(actualchildren.front().second);
		return ResultObject(CGALUtils::applyMinkowski(actualchildren));
	}

	CGAL_Nef_polyhedron *N = CGALUtils::applyOperator(children, op);
	// FIXME: Clarify when we can return nullptr and what that means
	if (!N) N = new CGAL_Nef_polyhedron;
	return ResultObject(N);
}

Geometry *CGALNefEvaluator::applyHull3D(const AbstractNode &node)
{
	Geometry::Geometries children = collectChildren3D(node);

	PolySet *P = new PolySet(3);
	if (CGALUtils::applyHull(children, *P)) {
		return P;
	}
	delete P;
	return nullptr;
}

/*!
	Since we can generate both Nef and non-Nef geometry, we need to insert it into
	the appropriate cache.
	This method inserts the geometry into the appropriate cache if it's not already cached.
*/
void CGALNefEvaluator::smartCacheInsert(const AbstractNode &node,
																			const shared_ptr<const Geometry> &geom)
{
	const std::string &key = this->tree.getIdString(node);

	shared_ptr<const CGAL_Nef_polyhedron> N = dynamic_pointer_cast<const CGAL_Nef_polyhedron>(geom);
	if (N) {
		if (!CGALCache::instance()->contains(key)) CGALCache::instance()->insert(key, N);
	}
	else {
		if (!GeometryCache::instance()->contains(key)) {
			if (!GeometryCache::instance()->insert(key, geom)) {
				PRINT("WARNING: CGALNefEvaluator: Node didn't fit into cache");
			}
		}
	}
}

bool CGALNefEvaluator::isSmartCached(const AbstractNode &node)
{
	const std::string &key = this->tree.getIdString(node);
	return (GeometryCache::instance()->contains(key) ||
					CGALCache::instance()->contains(key));
}

shared_ptr<const Geometry> CGALNefEvaluator::smartCacheGet(const AbstractNode &node, bool preferNef)
{
	const std::string &key = this->tree.getIdString(node);
	shared_ptr<const Geometry> geom;
	bool hasgeom = GeometryCache::instance()->contains(key);
	bool hascgal = CGALCache::instance()->contains(key);
	if (hascgal && (preferNef || !hasgeom)) geom = CGALCache::instance()->get(key);
	else if (hasgeom) geom = GeometryCache::instance()->get(key);
	return geom;
}

/*!
   RenderNodes just pass on convexity
*/
Response CGALNefEvaluator::visit(State &state, const RenderNode &node)
{
	if (state.isPrefix()) {
		if (isSmartCached(node)) return Response::PruneTraversal;
		state.setPreferNef(true); // Improve quality of CSG by avoiding conversion loss
	}
	if (state.isPostfix()) {
		shared_ptr<const class Geometry> geom;
		if (!isSmartCached(node)) {
			ResultObject res = applyToChildren(node, OpenSCADOperator::UNION);

			geom = res.constptr();
			if (shared_ptr<const PolySet> ps = dynamic_pointer_cast<const PolySet>(geom)) {
				// If we got a const object, make a copy
				shared_ptr<PolySet> newps;
				if (res.isConst()) newps.reset(new PolySet(*ps));
				else newps = dynamic_pointer_cast<PolySet>(res.ptr());
				newps->setConvexity(node.convexity);
				geom = newps;
			}
			else if (shared_ptr<const CGAL_Nef_polyhedron> N = dynamic_pointer_cast<const CGAL_Nef_polyhedron>(geom)) {
				// If we got a const object, make a copy
				shared_ptr<CGAL_Nef_polyhedron> newN;
				if (res.isConst()) newN.reset((CGAL_Nef_polyhedron*)N->copy());
				else newN = dynamic_pointer_cast<CGAL_Nef_polyhedron>(res.ptr());
				newN->setConvexity(node.convexity);
				geom = newN;
			}
		}
		else {
			geom = smartCacheGet(node, state.preferNef());
		}
		node.progress_report();
		addToParent(state, node, geom);
	}
	return Response::ContinueTraversal;
}



/*!
	input: List of 2D or 3D objects (not mixed)
	output: Polygon2d or 3D PolySet
	operation:
	  o Union all children
	  o Perform transform
 */			
Response CGALNefEvaluator::visit(State &state, const TransformNode &node)
{
	if (state.isPrefix() && isSmartCached(node)) return Response::PruneTraversal;
	if (state.isPostfix()) {
		shared_ptr<const class Geometry> geom;
		if (!isSmartCached(node)) {
			if (matrix_contains_infinity(node.matrix) || matrix_contains_nan(node.matrix)) {
				// due to the way parse/eval works we can't currently distinguish between NaN and Inf
				std::string loc = node.modinst->location().toRelativeString(this->tree.getDocumentPath());
				PRINTB("WARNING: Transformation matrix contains Not-a-Number and/or Infinity - removing object. %s", loc);
			}
			else {
				// First union all children
				ResultObject res = applyToChildren(node, OpenSCADOperator::UNION);
				if ((geom = res.constptr())) {
					if (geom->getDimension() == 2) {
						shared_ptr<const Polygon2d> polygons = dynamic_pointer_cast<const Polygon2d>(geom);
						assert(polygons);
						
						// If we got a const object, make a copy
						shared_ptr<Polygon2d> newpoly;
						if (res.isConst()) newpoly.reset(new Polygon2d(*polygons));
						else newpoly = dynamic_pointer_cast<Polygon2d>(res.ptr());
						
						Transform2d mat2;
						mat2.matrix() << 
							node.matrix(0,0), node.matrix(0,1), node.matrix(0,3),
							node.matrix(1,0), node.matrix(1,1), node.matrix(1,3),
							node.matrix(3,0), node.matrix(3,1), node.matrix(3,3);
						newpoly->transform(mat2);
						// A 2D transformation may flip the winding order of a polygon.
						// If that happens with a sanitized polygon, we need to reverse
						// the winding order for it to be correct.
						if (newpoly->isSanitized() && mat2.matrix().determinant() <= 0) {
							geom.reset(ClipperUtils::sanitize(*newpoly));
						}
					}
					else if (geom->getDimension() == 3) {
						shared_ptr<const PolySet> ps = dynamic_pointer_cast<const PolySet>(geom);
						if (ps) {
							// If we got a const object, make a copy
							shared_ptr<PolySet> newps;
							if (res.isConst()) newps.reset(new PolySet(*ps));
							else newps = dynamic_pointer_cast<PolySet>(res.ptr());
							newps->transform(node.matrix);
							geom = newps;
						}
						else {
							shared_ptr<const CGAL_Nef_polyhedron> N = dynamic_pointer_cast<const CGAL_Nef_polyhedron>(geom);
							assert(N);
							// If we got a const object, make a copy
							shared_ptr<CGAL_Nef_polyhedron> newN;
							if (res.isConst()) newN.reset((CGAL_Nef_polyhedron*)N->copy());
							else newN = dynamic_pointer_cast<CGAL_Nef_polyhedron>(res.ptr());
							newN->transform(node.matrix);
							geom = newN;
						}
					}
				}
			}
		}
		else {
			geom = smartCacheGet(node, state.preferNef());
		}
		addToParent(state, node, geom);
		node.progress_report();
	}
	return Response::ContinueTraversal;
}

/*!
	input: List of 3D objects
	output: Polygon2d
	operation:
	  o Union all children
		o Perform projection
 */			
Response CGALNefEvaluator::visit(State &state, const ProjectionNode &node)
{
	if (state.isPrefix() && isSmartCached(node)) return Response::PruneTraversal;
	if (state.isPostfix()) {
		shared_ptr<const class Geometry> geom;
		if (!isSmartCached(node)) {

			if (!node.cut_mode) {
				ClipperLib::Clipper sumclipper;
				for(const auto &item : this->visitedchildren[node.index()]) {
					const AbstractNode *chnode = item.first;
					const shared_ptr<const Geometry> &chgeom = item.second;
					// FIXME: Don't use deep access to modinst members
					if (chnode->modinst->isBackground()) continue;

					const Polygon2d *poly = nullptr;

// CGAL version of Geometry projection
// Causes crashes in createNefPolyhedronFromGeometry() for this model:
// projection(cut=false) {
//    cube(10);
//    difference() {
//      sphere(10);
//      cylinder(h=30, r=5, center=true);
//    }
// }
#if 0
					shared_ptr<const PolySet> chPS = dynamic_pointer_cast<const PolySet>(chgeom);
					const PolySet *ps2d = nullptr;
					shared_ptr<const CGAL_Nef_polyhedron> chN = dynamic_pointer_cast<const CGAL_Nef_polyhedron>(chgeom);
					if (chN) chPS.reset(chN->convertToPolyset());
					if (chPS) ps2d = PolysetUtils::flatten(*chPS);
					if (ps2d) {
						CGAL_Nef_polyhedron *N2d = CGALUtils::createNefPolyhedronFromGeometry(*ps2d);
						poly = N2d->convertToPolygon2d();
					}
#endif

// Clipper version of Geometry projection
// Clipper doesn't handle meshes very well.
// It's better in V6 but not quite there. FIXME: stand-alone example.
#if 1
					// project chgeom -> polygon2d
					shared_ptr<const PolySet> chPS = dynamic_pointer_cast<const PolySet>(chgeom);
					if (!chPS) {
						shared_ptr<const CGAL_Nef_polyhedron> chN = dynamic_pointer_cast<const CGAL_Nef_polyhedron>(chgeom);
						if (chN) {
							PolySet *ps = new PolySet(3);
							bool err = CGALUtils::createPolySetFromNefPolyhedron3(*chN->p3, *ps);
							if (err) {
								PRINT("ERROR: Nef->PolySet failed");
							}
							else {
								chPS.reset(ps);
							}
						}
					}
					if (chPS) poly = PolysetUtils::project(*chPS);
#endif

					if (poly) {
						ClipperLib::Paths result = ClipperUtils::fromPolygon2d(*poly);
						// Using NonZero ensures that we don't create holes from polygons sharing
						// edges since we're unioning a mesh
						result = ClipperUtils::process(result, 
																					 ClipperLib::ctUnion, 
																					 ClipperLib::pftNonZero);
						// Add correctly winded polygons to the main clipper
						sumclipper.AddPaths(result, ClipperLib::ptSubject, true);
					}

					delete poly;
				}
				ClipperLib::PolyTree sumresult;
				// This is key - without StrictlySimple, we tend to get self-intersecting results
				sumclipper.StrictlySimple(true);
				sumclipper.Execute(ClipperLib::ctUnion, sumresult, ClipperLib::pftNonZero, ClipperLib::pftNonZero);
				if (sumresult.Total() > 0) geom.reset(ClipperUtils::toPolygon2d(sumresult));
			}
			else {
				shared_ptr<const Geometry> newgeom = applyToChildren3D(node, OpenSCADOperator::UNION).constptr();
				if (newgeom) {
					shared_ptr<const CGAL_Nef_polyhedron> Nptr = dynamic_pointer_cast<const CGAL_Nef_polyhedron>(newgeom);
					if (!Nptr) {
						Nptr.reset(CGALUtils::createNefPolyhedronFromGeometry(*newgeom));
					}
					if (!Nptr->isEmpty()) {
						Polygon2d *poly = CGALUtils::project(*Nptr, node.cut_mode);
						if (poly) {
							poly->setConvexity(node.convexity);
							geom.reset(poly);
						}
					}
				}
			}
		}
		else {
			geom = smartCacheGet(node, false);
		}
		addToParent(state, node, geom);
		node.progress_report();
	}
	return Response::ContinueTraversal;
}		

/*!
	input: List of 2D or 3D objects (not mixed)
	output: any Geometry
	operation:
	  o Perform cgal operation
 */			
Response CGALNefEvaluator::visit(State &state, const CgaladvNode &node)
{
	if (state.isPrefix() && isSmartCached(node)) return Response::PruneTraversal;
	if (state.isPostfix()) {
		shared_ptr<const Geometry> geom;
		if (!isSmartCached(node)) {
			switch (node.type) {
			case CgaladvType::MINKOWSKI: {
				ResultObject res = applyToChildren(node, OpenSCADOperator::MINKOWSKI);
				geom = res.constptr();
				// If we added convexity, we need to pass it on
				if (geom && geom->getConvexity() != node.convexity) {
					shared_ptr<Geometry> editablegeom;
					// If we got a const object, make a copy
					if (res.isConst()) editablegeom.reset(geom->copy());
					else editablegeom = res.ptr();
					geom = editablegeom;
					editablegeom->setConvexity(node.convexity);
				}
				break;
			}
			case CgaladvType::HULL: {
				geom = applyToChildren(node, OpenSCADOperator::HULL).constptr();
				break;
			}
			case CgaladvType::RESIZE: {
				ResultObject res = applyToChildren(node, OpenSCADOperator::UNION);
				geom = res.constptr();
				if (geom) {
					shared_ptr<Geometry> editablegeom;
					// If we got a const object, make a copy
					if (res.isConst()) editablegeom.reset(geom->copy());
					else editablegeom = res.ptr();
					geom = editablegeom;

					shared_ptr<CGAL_Nef_polyhedron> N = dynamic_pointer_cast<CGAL_Nef_polyhedron>(editablegeom);
					if (N) {
						N->resize(node.newsize, node.autosize);
					}
					else {
						shared_ptr<Polygon2d> poly = dynamic_pointer_cast<Polygon2d>(editablegeom);
						if (poly) {
							poly->resize(Vector2d(node.newsize[0], node.newsize[1]),
													 Eigen::Matrix<bool,2,1>(node.autosize[0], node.autosize[1]));
						}
						else {
							shared_ptr<PolySet> ps = dynamic_pointer_cast<PolySet>(editablegeom);
							if (ps) {
								ps->resize(node.newsize, node.autosize);
							}
							else {
								assert(false);
							}
						}
					}
				}
				break;
			}
			default:
				assert(false && "not implemented");
			}
		}
		else {
			geom = smartCacheGet(node, state.preferNef());
		}
		addToParent(state, node, geom);
		node.progress_report();
	}
	return Response::ContinueTraversal;
}

