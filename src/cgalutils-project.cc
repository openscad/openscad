// this file is split into many separate cgalutils* files
// in order to workaround gcc 4.9.1 crashing on systems with only 2GB of RAM

#ifdef ENABLE_CGAL

#include "cgalutils.h"
#include "polyset.h"
#include "printutils.h"
#include "Polygon2d.h"
#include "polyset-utils.h"
#include "grid.h"
#include "node.h"

#include "cgal.h"
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/normal_vector_newell_3.h>
#include <CGAL/Handle_hash_function.h>

#include <CGAL/config.h> 
#include <CGAL/version.h> 

// Apply CGAL bugfix for CGAL-4.5.x
#if CGAL_VERSION_NR > CGAL_VERSION_NUMBER(4,5,1) || CGAL_VERSION_NR < CGAL_VERSION_NUMBER(4,5,0) 
#include <CGAL/convex_hull_3.h>
#else
#include "convex_hull_3_bugfix.h"
#endif

#include "svg.h"
#include "GeometryUtils.h"

#include <map>
#include <queue>

static void add_outline_to_poly(CGAL_Nef_polyhedron2::Explorer &explorer,
								CGAL_Nef_polyhedron2::Explorer::Halfedge_around_face_const_circulator circ,
								CGAL_Nef_polyhedron2::Explorer::Halfedge_around_face_const_circulator end,
								bool positive,
								Polygon2d *poly) {
	Outline2d outline;

	CGAL_For_all(circ, end) {
		if (explorer.is_standard(explorer.target(circ))) {
			CGAL_Nef_polyhedron2::Explorer::Point ep = explorer.point(explorer.target(circ));
			outline.vertices.push_back(Vector2d(to_double(ep.x()),
												to_double(ep.y())));
		}
	}

	if (!outline.vertices.empty()) {
		outline.positive = positive;
		poly->addOutline(outline);
	}
}

static Polygon2d *convertToPolygon2d(const CGAL_Nef_polyhedron2 &p2)
{
	Polygon2d *poly = new Polygon2d;
	
	typedef CGAL_Nef_polyhedron2::Explorer Explorer;
	typedef Explorer::Face_const_iterator fci_t;
	typedef Explorer::Halfedge_around_face_const_circulator heafcc_t;
	Explorer E = p2.explorer();
	for (fci_t fit = E.faces_begin(), facesend = E.faces_end(); fit != facesend; ++fit) {
		if (!fit->mark()) continue;
			heafcc_t fcirc(E.face_cycle(fit)), fend(fcirc);
			add_outline_to_poly(E, fcirc, fend, true, poly);
			for (CGAL_Nef_polyhedron2::Explorer::Hole_const_iterator j = E.holes_begin(fit);j != E.holes_end(fit); ++j) {
				CGAL_Nef_polyhedron2::Explorer::Halfedge_around_face_const_circulator hcirc(j), hend(hcirc);
				add_outline_to_poly(E, hcirc, hend, false, poly);
			}
	}
	poly->setSanitized(true);
	return poly;
}

/*

ZRemover

This class converts one or more Nef3 polyhedra into a Nef2 polyhedron by
stripping off the 'z' coordinates from the vertices. The resulting Nef2
poly is accumulated in the 'output_nefpoly2d' member variable.

The 'z' coordinates will either be all 0s, for an xy-plane intersected Nef3,
or, they will be a mixture of -eps and +eps, for a thin-box intersected Nef3.

Notes on CGAL's Nef Polyhedron2:

1. The 'mark' on a 2d Nef face is important when doing unions/intersections.
 If the 'mark' of a face is wrong the resulting nef2 poly will be unexpected.
2. The 'mark' can be dependent on the points fed to the Nef2 constructor.
 This is why we iterate through the 3d faces using the halfedge cycle
 source()->target() instead of the ordinary source()->source(). The
 the latter can generate sequences of points that will fail the
 the CGAL::is_simple_2() test, resulting in improperly marked nef2 polys.
3. 3d facets have 'two sides'. we throw out the 'down' side to prevent dups.

The class uses the 'visitor' pattern from the CGAL manual. See also
http://www.cgal.org/Manual/latest/doc_html/cgal_manual/Nef_3/Chapter_main.html
http://www.cgal.org/Manual/latest/doc_html/cgal_manual/Nef_3_ref/Class_Nef_polyhedron3.html
OGL_helper.h
*/

class ZRemover {
public:
	CGAL_Nef_polyhedron2::Boundary boundary;
	shared_ptr<CGAL_Nef_polyhedron2> tmpnef2d;
	shared_ptr<CGAL_Nef_polyhedron2> output_nefpoly2d;
	CGAL::Direction_3<CGAL_Kernel3> up;
	ZRemover()
	{
		output_nefpoly2d.reset( new CGAL_Nef_polyhedron2() );
		boundary = CGAL_Nef_polyhedron2::INCLUDED;
		up = CGAL::Direction_3<CGAL_Kernel3>(0,0,1);
	}
	void visit( CGAL_Nef_polyhedron3::Vertex_const_handle ) {}
	void visit( CGAL_Nef_polyhedron3::Halfedge_const_handle ) {}
	void visit( CGAL_Nef_polyhedron3::SHalfedge_const_handle ) {}
	void visit( CGAL_Nef_polyhedron3::SHalfloop_const_handle ) {}
	void visit( CGAL_Nef_polyhedron3::SFace_const_handle ) {}
	void visit( CGAL_Nef_polyhedron3::Halffacet_const_handle hfacet );
};


void ZRemover::visit(CGAL_Nef_polyhedron3::Halffacet_const_handle hfacet)
{
	PRINTDB(" <!-- ZRemover Halffacet visit. Mark: %i --> ",hfacet->mark());
	if (hfacet->plane().orthogonal_direction() != this->up) {
		PRINTD("  <!-- ZRemover down-facing half-facet. skipping -->");
		PRINTD(" <!-- ZRemover Halffacet visit end-->");
		return;
	}

	// possible optimization - throw out facets that are vertically oriented

	CGAL_Nef_polyhedron3::Halffacet_cycle_const_iterator fci;
	int contour_counter = 0;
	CGAL_forall_facet_cycles_of(fci, hfacet) {
		if (fci.is_shalfedge()) {
			PRINTD(" <!-- ZRemover Halffacet cycle begin -->");
			CGAL_Nef_polyhedron3::SHalfedge_around_facet_const_circulator c1(fci), cend(c1);
			std::vector<CGAL_Nef_polyhedron2::Explorer::Point> contour;
			CGAL_For_all(c1, cend) {
				CGAL_Nef_polyhedron3::Point_3 point3d = c1->source()->target()->point();
				CGAL_Nef_polyhedron2::Explorer::Point point2d(CGAL::to_double(point3d.x()),
																											CGAL::to_double(point3d.y()));
				contour.push_back(point2d);
			}
			if (contour.size()==0) continue;

			if (OpenSCAD::debug!="")
				PRINTDB(" <!-- is_simple_2: %i -->", CGAL::is_simple_2(contour.begin(), contour.end()));

			tmpnef2d.reset(new CGAL_Nef_polyhedron2(contour.begin(), contour.end(), boundary));

			if (contour_counter == 0) {
				PRINTDB(" <!-- contour is a body. make union(). %i  points -->", contour.size());
				*(output_nefpoly2d) += *(tmpnef2d);
			} else {
				PRINTDB(" <!-- contour is a hole. make intersection(). %i  points -->", contour.size());
				*(output_nefpoly2d) *= *(tmpnef2d);
			}

			/*log << "\n<!-- ======== output tmp nef: ==== -->\n"
				<< OpenSCAD::dump_svg(*tmpnef2d) << "\n"
				<< "\n<!-- ======== output accumulator: ==== -->\n"
				<< OpenSCAD::dump_svg(*output_nefpoly2d) << "\n";*/

			contour_counter++;
		} else {
			PRINTD(" <!-- ZRemover trivial facet cycle skipped -->");
		}
		PRINTD(" <!-- ZRemover Halffacet cycle end -->");
	}
	PRINTD(" <!-- ZRemover Halffacet visit end -->");
}



namespace CGALUtils {

	Polygon2d *project(const CGAL_Nef_polyhedron &N, bool cut)
	{
		Polygon2d *poly = nullptr;
		if (N.getDimension() != 3) return poly;

		CGAL_Nef_polyhedron newN;
		if (cut) {
			CGAL::Failure_behaviour old_behaviour = CGAL::set_error_behaviour(CGAL::THROW_EXCEPTION);
			try {
				CGAL_Nef_polyhedron3::Plane_3 xy_plane = CGAL_Nef_polyhedron3::Plane_3(0,0,1,0);
				newN.p3.reset(new CGAL_Nef_polyhedron3(N.p3->intersection(xy_plane, CGAL_Nef_polyhedron3::PLANE_ONLY)));
			}
			catch (const CGAL::Failure_exception &e) {
				PRINTDB("CGALUtils::project during plane intersection: %s", e.what());
				try {
					PRINTD("Trying alternative intersection using very large thin box: ");
					std::vector<CGAL_Point_3> pts;
					// dont use z of 0. there are bugs in CGAL.
					double inf = 1e8;
					double eps = 0.001;
					CGAL_Point_3 minpt(-inf, -inf, -eps);
					CGAL_Point_3 maxpt( inf,  inf,  eps);
					CGAL_Iso_cuboid_3 bigcuboid(minpt, maxpt);
					for (int i=0;i<8;i++) pts.push_back(bigcuboid.vertex(i));
					CGAL_Polyhedron bigbox;
					CGAL::convex_hull_3(pts.begin(), pts.end(), bigbox);
					CGAL_Nef_polyhedron3 nef_bigbox(bigbox);
					newN.p3.reset(new CGAL_Nef_polyhedron3(nef_bigbox.intersection(*N.p3)));
				}
				catch (const CGAL::Failure_exception &e) {
					PRINTB("ERROR: CGAL error in CGALUtils::project during bigbox intersection: %s", e.what());
				}
			}
				
			if (!newN.p3 || newN.p3->is_empty()) {
				CGAL::set_error_behaviour(old_behaviour);
				PRINT("WARNING: projection() failed.");
				return poly;
			}
				
			PRINTDB("%s",OpenSCAD::svg_header(480, 100000));
			try {
				ZRemover zremover;
				CGAL_Nef_polyhedron3::Volume_const_iterator i;
				CGAL_Nef_polyhedron3::Shell_entry_const_iterator j;
				CGAL_Nef_polyhedron3::SFace_const_handle sface_handle;
				for (i = newN.p3->volumes_begin(); i != newN.p3->volumes_end(); ++i) {
					PRINTDB("<!-- volume. mark: %s -->",i->mark());
					for (j = i->shells_begin(); j != i->shells_end(); ++j) {
						PRINTDB("<!-- shell. (vol mark was: %i)", i->mark());;
						sface_handle = CGAL_Nef_polyhedron3::SFace_const_handle(j);
						newN.p3->visit_shell_objects(sface_handle , zremover);
						PRINTD("<!-- shell. end. -->");
					}
					PRINTD("<!-- volume end. -->");
				}
				poly = convertToPolygon2d(*zremover.output_nefpoly2d);
			}	catch (const CGAL::Failure_exception &e) {
				PRINTB("ERROR: CGAL error in CGALUtils::project while flattening: %s", e.what());
			}
			PRINTD("</svg>");
				
			CGAL::set_error_behaviour(old_behaviour);
		}
		// In projection mode all the triangles are projected manually into the XY plane
		else {
			PolySet ps(3);
			bool err = CGALUtils::createPolySetFromNefPolyhedron3(*N.p3, ps);
			if (err) {
				PRINT("ERROR: Nef->PolySet failed");
				return poly;
			}
			poly = PolysetUtils::project(ps);
		}
		return poly;
	}

} // namespace

#endif // ENABLE_CGAL
