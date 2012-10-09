#include "PolySetCGALEvaluator.h"
#include "cgal.h"
#include "cgalutils.h"
#include "polyset.h"
#include "CGALEvaluator.h"
#include "projectionnode.h"
#include "linearextrudenode.h"
#include "rotateextrudenode.h"
#include "cgaladvnode.h"
#include "rendernode.h"
#include "dxfdata.h"
#include "dxftess.h"
#include "module.h"

#include "printutils.h"
#include "openscad.h" // get_fragments_from_r()
#include <boost/foreach.hpp>

/*

This Visitor is used in the 'cut' process. Essentially, one or more of
our 3d nef polyhedrons have been 'cut' by the xy-plane, forming a number
of polygons that are essentially 'flat'. This Visitor object, when
called repeatedly, collects those flat polygons together and forms a new
2d nef polyhedron out of them. It keeps track of the 'holes' so that
in the final resulting polygon, they are preserved properly.

The output polygon is stored in the output_nefpoly2d variable.

For more information on 3d + 2d nef polyhedrons, facets, halffacets,
facet cycles, etc, please see these websites:

http://www.cgal.org/Manual/latest/doc_html/cgal_manual/Nef_3/Chapter_main.html
http://www.cgal.org/Manual/latest/doc_html/cgal_manual/Nef_3_ref/Class_Nef_polyhedron_3-Traits---Halffacet.html

The halffacet iteration 'circulator' code is based on OGL_helper.h

Why do we throw out all 'down' half-facets? Imagine a triangle in 3d
space - it has one 'half face' for one 'side' and another 'half face' for the
other 'side'. If we iterated over both half-faces we would get the same vertex
coordinates twice. Instead, we only need one side, in our case, we
chose the 'up' side.
*/
class NefShellVisitor_for_cut {
public:
	std::stringstream out;
	CGAL_Nef_polyhedron2::Boundary boundary;
	shared_ptr<CGAL_Nef_polyhedron2> tmpnef2d;
	shared_ptr<CGAL_Nef_polyhedron2> output_nefpoly2d;
	CGAL::Direction_3<CGAL_Kernel3> up;
	bool debug;
	NefShellVisitor_for_cut(bool debug=false)
	{
		output_nefpoly2d.reset( new CGAL_Nef_polyhedron2() );
		boundary = CGAL_Nef_polyhedron2::INCLUDED;
		up = CGAL::Direction_3<CGAL_Kernel3>(0,0,1);
		this->debug = debug;
	}
	std::string dump()
	{
		return out.str();
	}
	void visit( CGAL_Nef_polyhedron3::Vertex_const_handle ) {}
	void visit( CGAL_Nef_polyhedron3::Halfedge_const_handle ) {}
	void visit( CGAL_Nef_polyhedron3::SHalfedge_const_handle ) {}
	void visit( CGAL_Nef_polyhedron3::SHalfloop_const_handle ) {}
	void visit( CGAL_Nef_polyhedron3::SFace_const_handle ) {}
	void visit( CGAL_Nef_polyhedron3::Halffacet_const_handle hfacet ) {
		if ( hfacet->plane().orthogonal_direction() != this->up ) {
			if (debug) out << "down facing half-facet. skipping\n";
			return;
		}

		int numcontours = 0;
		CGAL_Nef_polyhedron3::Halffacet_cycle_const_iterator i;
		CGAL_forall_facet_cycles_of( i, hfacet ) {
			CGAL_Nef_polyhedron3::SHalfedge_around_facet_const_circulator c1(i), c2(c1);
			std::list<CGAL_Nef_polyhedron2::Point> contour;
			CGAL_For_all( c1, c2 ) {
				CGAL_Nef_polyhedron3::Point_3 point3d = c1->source()->source()->point();
				CGAL_Nef_polyhedron2::Point point2d( point3d.x(), point3d.y() );
				contour.push_back( point2d );
			}
			tmpnef2d.reset( new CGAL_Nef_polyhedron2( contour.begin(), contour.end(), boundary ) );
			if ( numcontours == 0 ) {
				if (debug) out << " contour is a body. make union(). " << contour.size() << " points.\n" ;
				*output_nefpoly2d += *tmpnef2d;
			} else {
				if (debug) out << " contour is a hole. make intersection(). " << contour.size() << " points.\n";
				*output_nefpoly2d *= *tmpnef2d;
			}
			numcontours++;
		} // next facet cycle (i.e. next contour)
	} // visit()
};

PolySetCGALEvaluator::PolySetCGALEvaluator(CGALEvaluator &cgalevaluator)
	: PolySetEvaluator(cgalevaluator.getTree()), cgalevaluator(cgalevaluator)
{
	this->debug = false;
}

PolySet *PolySetCGALEvaluator::evaluatePolySet(const ProjectionNode &node)
{
	// Before projecting, union all children
	CGAL_Nef_polyhedron sum;
	BOOST_FOREACH (AbstractNode * v, node.getChildren()) {
		if (v->modinst->isBackground()) continue;
		CGAL_Nef_polyhedron N = this->cgalevaluator.evaluateCGALMesh(*v);
		if (N.dim == 3) {
			if (sum.empty()) sum = N.copy();
			else sum += N;
		}
	}
	if (sum.empty()) return NULL;
	if (!sum.p3->is_simple()) {
		if (!node.cut_mode) {
			PRINT("WARNING: Body of projection(cut = false) isn't valid 2-manifold! Modify your design..");
			return new PolySet();
		}
	}

	CGAL_Nef_polyhedron nef_poly;

	if (node.cut_mode) {
		CGAL::Failure_behaviour old_behaviour = CGAL::set_error_behaviour(CGAL::THROW_EXCEPTION);
		try {
			// intersect 'sum' with the x-y plane
			CGAL_Nef_polyhedron3::Plane_3 xy_plane = CGAL_Nef_polyhedron3::Plane_3( 0,0,1,0 );
			*sum.p3 = sum.p3->intersection( xy_plane, CGAL_Nef_polyhedron3::PLANE_ONLY);
			
			// Visit each polygon in sum.p3 and union/intersect into a 2d polygon (with holes)
			// For info on Volumes, Shells, Facets, and the 'visitor' pattern, please see
			// http://www.cgal.org/Manual/latest/doc_html/cgal_manual/Nef_3/Chapter_main.html
			NefShellVisitor_for_cut shell_visitor;
			CGAL_Nef_polyhedron3::Volume_const_iterator i;
			CGAL_Nef_polyhedron3::Shell_entry_const_iterator j;
			CGAL_Nef_polyhedron3::SFace_const_handle sface_handle;
			for ( i = sum.p3->volumes_begin(); i != sum.p3->volumes_end(); ++i ) {
				for ( j = i->shells_begin(); j != i->shells_end(); ++j ) {
					sface_handle = CGAL_Nef_polyhedron3::SFace_const_handle( j );
					sum.p3->visit_shell_objects( sface_handle , shell_visitor );
				}
			}
			if (debug) std::cout << "shell visitor\n" << shell_visitor.dump() << "\nshell visitor end\n";
			
			nef_poly.p2 = shell_visitor.output_nefpoly2d;
			nef_poly.dim = 2;
			if (debug) std::cout << "--\n" << nef_poly.dump_p2() << "\n";
		}
		catch (const CGAL::Failure_exception &e) {
			PRINTB("CGAL error in projection node: %s", e.what());
			return NULL;
		}

		// Extract polygons in the XY plane, ignoring all other polygons
		// FIXME: If the polyhedron is really thin, there might be unwanted polygons
		// in the XY plane, causing the resulting 2D polygon to be self-intersection
		// and cause a crash in CGALEvaluator::PolyReducer. The right solution is to
		// filter these polygons here. kintel 20120203.
		/*
		Grid2d<unsigned int> conversion_grid(GRID_COARSE);
		for (size_t i = 0; i < ps3->polygons.size(); i++) {
			for (size_t j = 0; j < ps3->polygons[i].size(); j++) {
				double x = ps3->polygons[i][j][0];
				double y = ps3->polygons[i][j][1];
				double z = ps3->polygons[i][j][2];
				if (z != 0)
					goto next_ps3_polygon_cut_mode;
				if (conversion_grid.align(x, y) == i+1)
					goto next_ps3_polygon_cut_mode;
				conversion_grid.data(x, y) = i+1;
			}
			ps->append_poly();
			for (size_t j = 0; j < ps3->polygons[i].size(); j++) {
				double x = ps3->polygons[i][j][0];
				double y = ps3->polygons[i][j][1];
				conversion_grid.align(x, y);
				ps->insert_vertex(x, y);
			}
		next_ps3_polygon_cut_mode:;
		}
		*/
	}
	// In projection mode all the triangles are projected manually into the XY plane
	else
	{
		PolySet *ps3 = sum.convertToPolyset();
		if (!ps3) return NULL;
		for (size_t i = 0; i < ps3->polygons.size(); i++)
		{
			int min_x_p = -1;
			double min_x_val = 0;
			for (size_t j = 0; j < ps3->polygons[i].size(); j++) {
				double x = ps3->polygons[i][j][0];
				if (min_x_p < 0 || x < min_x_val) {
					min_x_p = j;
					min_x_val = x;
				}
			}
			int min_x_p1 = (min_x_p+1) % ps3->polygons[i].size();
			int min_x_p2 = (min_x_p+ps3->polygons[i].size()-1) % ps3->polygons[i].size();
			double ax = ps3->polygons[i][min_x_p1][0] - ps3->polygons[i][min_x_p][0];
			double ay = ps3->polygons[i][min_x_p1][1] - ps3->polygons[i][min_x_p][1];
			double at = atan2(ay, ax);
			double bx = ps3->polygons[i][min_x_p2][0] - ps3->polygons[i][min_x_p][0];
			double by = ps3->polygons[i][min_x_p2][1] - ps3->polygons[i][min_x_p][1];
			double bt = atan2(by, bx);

			double eps = 0.000001;
			if (fabs(at - bt) < eps || (fabs(ax) < eps && fabs(ay) < eps) ||
					(fabs(bx) < eps && fabs(by) < eps)) {
				// this triangle is degenerated in projection
				continue;
			}

			std::list<CGAL_Nef_polyhedron2::Point> plist;
			for (size_t j = 0; j < ps3->polygons[i].size(); j++) {
				double x = ps3->polygons[i][j][0];
				double y = ps3->polygons[i][j][1];
				CGAL_Nef_polyhedron2::Point p = CGAL_Nef_polyhedron2::Point(x, y);
				if (at > bt)
					plist.push_front(p);
				else
					plist.push_back(p);
			}
			// FIXME: Should the CGAL_Nef_polyhedron2 be cached?
			if (nef_poly.empty()) {
				nef_poly.dim = 2;
				nef_poly.p2.reset(new CGAL_Nef_polyhedron2(plist.begin(), plist.end(), CGAL_Nef_polyhedron2::INCLUDED));
			}
			else {
				(*nef_poly.p2) += CGAL_Nef_polyhedron2(plist.begin(), plist.end(), CGAL_Nef_polyhedron2::INCLUDED);
			}
		}
		delete ps3;
	}

	PolySet *ps = nef_poly.convertToPolyset();
	assert( ps != NULL );
	ps->convexity = node.convexity;
	if (debug) std::cout << "--\n" << ps->dump() << "\n";

	return ps;
}

static void add_slice(PolySet *ps, const DxfData &dxf, DxfData::Path &path, double rot1, double rot2, double h1, double h2)
{
	bool splitfirst = sin(rot2 - rot1) >= 0.0;
	for (size_t j = 1; j < path.indices.size(); j++)
	{
		int k = j - 1;

		double jx1 = dxf.points[path.indices[j]][0] *  cos(rot1*M_PI/180) + dxf.points[path.indices[j]][1] * sin(rot1*M_PI/180);
		double jy1 = dxf.points[path.indices[j]][0] * -sin(rot1*M_PI/180) + dxf.points[path.indices[j]][1] * cos(rot1*M_PI/180);

		double jx2 = dxf.points[path.indices[j]][0] *  cos(rot2*M_PI/180) + dxf.points[path.indices[j]][1] * sin(rot2*M_PI/180);
		double jy2 = dxf.points[path.indices[j]][0] * -sin(rot2*M_PI/180) + dxf.points[path.indices[j]][1] * cos(rot2*M_PI/180);

		double kx1 = dxf.points[path.indices[k]][0] *  cos(rot1*M_PI/180) + dxf.points[path.indices[k]][1] * sin(rot1*M_PI/180);
		double ky1 = dxf.points[path.indices[k]][0] * -sin(rot1*M_PI/180) + dxf.points[path.indices[k]][1] * cos(rot1*M_PI/180);

		double kx2 = dxf.points[path.indices[k]][0] *  cos(rot2*M_PI/180) + dxf.points[path.indices[k]][1] * sin(rot2*M_PI/180);
		double ky2 = dxf.points[path.indices[k]][0] * -sin(rot2*M_PI/180) + dxf.points[path.indices[k]][1] * cos(rot2*M_PI/180);

		if (splitfirst)
		{
			ps->append_poly();
			if (path.is_inner) {
				ps->append_vertex(kx1, ky1, h1);
				ps->append_vertex(jx1, jy1, h1);
				ps->append_vertex(jx2, jy2, h2);
			} else {
				ps->insert_vertex(kx1, ky1, h1);
				ps->insert_vertex(jx1, jy1, h1);
				ps->insert_vertex(jx2, jy2, h2);
			}

			ps->append_poly();
			if (path.is_inner) {
				ps->append_vertex(kx2, ky2, h2);
				ps->append_vertex(kx1, ky1, h1);
				ps->append_vertex(jx2, jy2, h2);
			} else {
				ps->insert_vertex(kx2, ky2, h2);
				ps->insert_vertex(kx1, ky1, h1);
				ps->insert_vertex(jx2, jy2, h2);
			}
		}
		else
		{
			ps->append_poly();
			if (path.is_inner) {
				ps->append_vertex(kx1, ky1, h1);
				ps->append_vertex(jx1, jy1, h1);
				ps->append_vertex(kx2, ky2, h2);
			} else {
				ps->insert_vertex(kx1, ky1, h1);
				ps->insert_vertex(jx1, jy1, h1);
				ps->insert_vertex(kx2, ky2, h2);
			}

			ps->append_poly();
			if (path.is_inner) {
				ps->append_vertex(jx2, jy2, h2);
				ps->append_vertex(kx2, ky2, h2);
				ps->append_vertex(jx1, jy1, h1);
			} else {
				ps->insert_vertex(jx2, jy2, h2);
				ps->insert_vertex(kx2, ky2, h2);
				ps->insert_vertex(jx1, jy1, h1);
			}
		}
	}
}

PolySet *PolySetCGALEvaluator::evaluatePolySet(const LinearExtrudeNode &node)
{
	DxfData *dxf;

	if (node.filename.empty())
	{
		// Before extruding, union all (2D) children nodes
		// to a single DxfData, then tesselate this into a PolySet
		CGAL_Nef_polyhedron sum;
		BOOST_FOREACH (AbstractNode * v, node.getChildren()) {
			if (v->modinst->isBackground()) continue;
			CGAL_Nef_polyhedron N = this->cgalevaluator.evaluateCGALMesh(*v);
			if (!N.empty()) {
				if (N.dim != 2) {
					PRINT("ERROR: linear_extrude() is not defined for 3D child objects!");
				}
				else {
					if (sum.empty()) sum = N.copy();
					else sum += N;
				}
			}
		}

		if (sum.empty()) return NULL;
		dxf = sum.convertToDxfData();;
	} else {
		dxf = new DxfData(node.fn, node.fs, node.fa, node.filename, node.layername, node.origin_x, node.origin_y, node.scale);
	}

	PolySet *ps = extrudeDxfData(node, *dxf);
	delete dxf;
	return ps;
}

PolySet *PolySetCGALEvaluator::extrudeDxfData(const LinearExtrudeNode &node, DxfData &dxf)
{
	PolySet *ps = new PolySet();
	ps->convexity = node.convexity;

	double h1, h2;

	if (node.center) {
		h1 = -node.height/2.0;
		h2 = +node.height/2.0;
	} else {
		h1 = 0;
		h2 = node.height;
	}

	bool first_open_path = true;
	for (size_t i = 0; i < dxf.paths.size(); i++)
	{
		if (dxf.paths[i].is_closed)
			continue;
		if (first_open_path) {
			PRINTB("WARNING: Open paths in dxf_linear_extrude(file = \"%s\", layer = \"%s\"):",
					node.filename % node.layername);
			first_open_path = false;
		}
		PRINTB("   %9.5f %10.5f ... %10.5f %10.5f",
					 (dxf.points[dxf.paths[i].indices.front()][0] / node.scale + node.origin_x) %
					 (dxf.points[dxf.paths[i].indices.front()][1] / node.scale + node.origin_y) %
					 (dxf.points[dxf.paths[i].indices.back()][0] / node.scale + node.origin_x) %
					 (dxf.points[dxf.paths[i].indices.back()][1] / node.scale + node.origin_y));
	}


	if (node.has_twist)
	{
		dxf_tesselate(ps, dxf, 0, false, true, h1);
		dxf_tesselate(ps, dxf, node.twist, true, true, h2);
		for (int j = 0; j < node.slices; j++)
		{
			double t1 = node.twist*j / node.slices;
			double t2 = node.twist*(j+1) / node.slices;
			double g1 = h1 + (h2-h1)*j / node.slices;
			double g2 = h1 + (h2-h1)*(j+1) / node.slices;
			for (size_t i = 0; i < dxf.paths.size(); i++)
			{
				if (!dxf.paths[i].is_closed)
					continue;
				add_slice(ps, dxf, dxf.paths[i], t1, t2, g1, g2);
			}
		}
	}
	else
	{
		dxf_tesselate(ps, dxf, 0, false, true, h1);
		dxf_tesselate(ps, dxf, 0, true, true, h2);
		for (size_t i = 0; i < dxf.paths.size(); i++)
		{
			if (!dxf.paths[i].is_closed)
				continue;
			add_slice(ps, dxf, dxf.paths[i], 0, 0, h1, h2);
		}
	}

	return ps;
}

PolySet *PolySetCGALEvaluator::evaluatePolySet(const RotateExtrudeNode &node)
{
	DxfData *dxf;

	if (node.filename.empty())
	{
		// Before extruding, union all (2D) children nodes
		// to a single DxfData, then tesselate this into a PolySet
		CGAL_Nef_polyhedron sum;
		BOOST_FOREACH (AbstractNode * v, node.getChildren()) {
			if (v->modinst->isBackground()) continue;
			CGAL_Nef_polyhedron N = this->cgalevaluator.evaluateCGALMesh(*v);
			if (!N.empty()) {
				if (N.dim != 2) {
					PRINT("ERROR: rotate_extrude() is not defined for 3D child objects!");
				}
				else {
					if (sum.empty()) sum = N.copy();
					else sum += N;
				}
			}
		}

		if (sum.empty()) return NULL;
		dxf = sum.convertToDxfData();
	} else {
		dxf = new DxfData(node.fn, node.fs, node.fa, node.filename, node.layername, node.origin_x, node.origin_y, node.scale);
	}

	PolySet *ps = rotateDxfData(node, *dxf);
	delete dxf;
	return ps;
}

PolySet *PolySetCGALEvaluator::evaluatePolySet(const CgaladvNode &node)
{
	CGAL_Nef_polyhedron N = this->cgalevaluator.evaluateCGALMesh(node);
	PolySet *ps = NULL;
	if (!N.empty()) {
		ps = N.convertToPolyset();
		if (ps) ps->convexity = node.convexity;
	}

	return ps;
}

PolySet *PolySetCGALEvaluator::evaluatePolySet(const RenderNode &node)
{
	CGAL_Nef_polyhedron N = this->cgalevaluator.evaluateCGALMesh(node);
	PolySet *ps = NULL;
	if (!N.empty()) {
		if (N.dim == 3 && !N.p3->is_simple()) {
			PRINT("WARNING: Body of render() isn't valid 2-manifold!");
		}
		else {
			ps = N.convertToPolyset();
			if (ps) ps->convexity = node.convexity;
		}
	}
	return ps;
}

PolySet *PolySetCGALEvaluator::rotateDxfData(const RotateExtrudeNode &node, DxfData &dxf)
{
	PolySet *ps = new PolySet();
	ps->convexity = node.convexity;

	for (size_t i = 0; i < dxf.paths.size(); i++)
	{
		double max_x = 0;
		for (size_t j = 0; j < dxf.paths[i].indices.size(); j++) {
			max_x = fmax(max_x, dxf.points[dxf.paths[i].indices[j]][0]);
		}

		int fragments = get_fragments_from_r(max_x, node.fn, node.fs, node.fa);

		double ***points;
		points = new double**[fragments];
		for (int j=0; j < fragments; j++) {
			points[j] = new double*[dxf.paths[i].indices.size()];
			for (size_t k=0; k < dxf.paths[i].indices.size(); k++)
				points[j][k] = new double[3];
		}

		for (int j = 0; j < fragments; j++) {
			double a = (j*2*M_PI) / fragments - M_PI/2; // start on the X axis
			for (size_t k = 0; k < dxf.paths[i].indices.size(); k++) {
				points[j][k][0] = dxf.points[dxf.paths[i].indices[k]][0] * sin(a);
			 	points[j][k][1] = dxf.points[dxf.paths[i].indices[k]][0] * cos(a);
				points[j][k][2] = dxf.points[dxf.paths[i].indices[k]][1];
			}
		}

		for (int j = 0; j < fragments; j++) {
			int j1 = j + 1 < fragments ? j + 1 : 0;
			for (size_t k = 0; k < dxf.paths[i].indices.size(); k++) {
				int k1 = k + 1 < dxf.paths[i].indices.size() ? k + 1 : 0;
				if (points[j][k][0] != points[j1][k][0] ||
						points[j][k][1] != points[j1][k][1] ||
						points[j][k][2] != points[j1][k][2]) {
					ps->append_poly();
					ps->append_vertex(points[j ][k ][0],
							points[j ][k ][1], points[j ][k ][2]);
					ps->append_vertex(points[j1][k ][0],
							points[j1][k ][1], points[j1][k ][2]);
					ps->append_vertex(points[j ][k1][0],
							points[j ][k1][1], points[j ][k1][2]);
				}
				if (points[j][k1][0] != points[j1][k1][0] ||
						points[j][k1][1] != points[j1][k1][1] ||
						points[j][k1][2] != points[j1][k1][2]) {
					ps->append_poly();
					ps->append_vertex(points[j ][k1][0],
							points[j ][k1][1], points[j ][k1][2]);
					ps->append_vertex(points[j1][k ][0],
							points[j1][k ][1], points[j1][k ][2]);
					ps->append_vertex(points[j1][k1][0],
							points[j1][k1][1], points[j1][k1][2]);
				}
			}
		}

		for (int j=0; j < fragments; j++) {
			for (size_t k=0; k < dxf.paths[i].indices.size(); k++)
				delete[] points[j][k];
			delete[] points[j];
		}
		delete[] points;
	}
	
	return ps;
}
