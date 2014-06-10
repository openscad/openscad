// Copyright (c) 1997-2002,2005 Max-Planck-Institute Saarbruecken (Germany).
// All rights reserved.
//
// This file is part of CGAL (www.cgal.org).
// You can redistribute it and/or modify it under the terms of the GNU
// General Public License as published by the Free Software Foundation,
// either version 3 of the License, or (at your option) any later version.
//
// Licensees holding a valid commercial license may use this file in
// accordance with the commercial license agreement provided with the software.
//
// This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
// WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
// $URL: svn+ssh://scm.gforge.inria.fr/svn/cgal/branches/releases/CGAL-4.0-branch/Nef_3/include/CGAL/Nef_helper_3.h $
// $Id: Nef_helper_3.h 67117 2012-01-13 18:14:48Z lrineau $
// 
//
// Author(s)     : Michael Seel    <seel@mpi-sb.mpg.de>
//                 Miguel Granados <granados@mpi-sb.mpg.de>
//                 Susan Hert      <hert@mpi-sb.mpg.de>
//                 Lutz Kettner    <kettner@mpi-sb.mpg.de>
//                 Ralf Osbild     <osbild@mpi-sb.mpg.de>
//                 Peter Hachenberger <hachenberger@mpi-sb.mpg.de>

/*
 modified by don bright for OpenSCAD, 2013.

This works around issue #410, where CGAL's Nef_Polyhedron3.convert_to_Polyhedron
throws an uncatchable exception, due to an CGAL_Assertion being thrown in
Polyhedron Incremental Builder's destructor while a Triangulation exception
is still active, resulting in program termination (crashing).

The purpose here is not to improve/change the way CGAL's Nef code works,
but instead to tweak it slightly to prevent OpenSCAD from crashing. The
behavior of the code should otherwise be exactly the same as CGAL's standard
code.

This file was created by copying three sections
from CGAL's Nef_polyhedron3.h that were protected/private:

 Triangulation_handler2
 Build_Polyhedron
 convert_to_polyhedron

Very small code changes have been made. First, there are many template
type specifiers added to enable the movement of the code to the outside
of the Nef Polyhedron class. Second, there is a try{}catch(...){} block
added in the Builder around the Triangulation code. Third, there is an error
variable added for non-Exception communication with the caller.

Eventually, if CGAL itself is updated and the update is widely
distributed, this file may become obsolete and can be deleted from OpenSCAD

*/


#pragma once

#include <CGAL/Polyhedron_incremental_builder_3.h>
#include <CGAL/Polyhedron_3.h>
#include <CGAL/assertions.h>

#include <CGAL/Constrained_triangulation_2.h>
#include <CGAL/Triangulation_data_structure_2.h>
#include <CGAL/Projection_traits_xy_3.h>
#include <CGAL/Projection_traits_yz_3.h>
#include <CGAL/Projection_traits_xz_3.h>
#include <CGAL/Constrained_triangulation_face_base_2.h>

#include <CGAL/exceptions.h> // added for OpenSCAD
#include "printutils.h"      // added for OpenSCAD

namespace nefworkaround {    // added for OpenSCAD

template<typename Kernel, typename Nef>
class Triangulation_handler2 {

    typedef typename CGAL::Triangulation_vertex_base_2<Kernel>               Vb;
    typedef typename CGAL::Constrained_triangulation_face_base_2<Kernel>     Fb;
    typedef typename CGAL::Triangulation_data_structure_2<Vb,Fb>             TDS;
    typedef typename CGAL::Constrained_triangulation_2<Kernel,TDS>           CT;

    typedef typename CT::Face_handle           Face_handle;
    typedef typename CT::Vertex_handle         CTVertex_handle;
    typedef typename CT::Finite_faces_iterator Finite_face_iterator;
    typedef typename CT::Edge                  Edge;
    CT ct;
    CGAL::Unique_hash_map<Face_handle, bool> visited;
    CGAL::Unique_hash_map<CTVertex_handle, typename Nef::Vertex_const_handle> ctv2v;
    Finite_face_iterator fi;
    typename Nef::Plane_3 supporting_plane;

  public:
    Triangulation_handler2(typename Nef::Halffacet_const_handle f) :
      visited(false), supporting_plane(f->plane()) {

      typename Nef::Halffacet_cycle_const_iterator fci;
      for(fci=f->facet_cycles_begin(); fci!=f->facet_cycles_end(); ++fci) {
	if(fci.is_shalfedge()) {
          typename Nef::SHalfedge_around_facet_const_circulator sfc(fci), send(sfc);
	  CGAL_For_all(sfc,send) {
            CGAL_NEF_TRACEN("  insert point" << sfc->source()->source()->point());
	    CTVertex_handle ctv = ct.insert(sfc->source()->source()->point());
	    ctv2v[ctv] = sfc->source()->source();
          }
        }
      }

      for(fci=f->facet_cycles_begin(); fci!=f->facet_cycles_end(); ++fci) {
	if(fci.is_shalfedge()) {
          typename Nef::SHalfedge_around_facet_const_circulator sfc(fci), send(sfc);
	  CGAL_For_all(sfc,send) {
            CGAL_NEF_TRACEN("  insert constraint" << sfc->source()->source()->point()
	                     << "->" << sfc->source()->twin()->source()->point());
	    ct.insert_constraint(sfc->source()->source()->point(),
	                         sfc->source()->twin()->source()->point());
          }
        }
      }
      CGAL_assertion(ct.is_valid());

      CGAL_NEF_TRACEN("number of finite triangles " << ct.number_of_faces());

      typename CT::Face_handle infinite = ct.infinite_face();
      typename CT::Vertex_handle ctv = infinite->vertex(1);
      if(ct.is_infinite(ctv)) ctv = infinite->vertex(2);
      CGAL_assertion(!ct.is_infinite(ctv));

      typename CT::Face_handle opposite;
      typename CT::Face_circulator vc(ctv,infinite);
      do { opposite = vc++;
      } while(!ct.is_constrained(typename CT::Edge(vc,vc->index(opposite))));
      typename CT::Face_handle first = vc;

      CGAL_assertion(!ct.is_infinite(first));
      traverse_triangulation(first, first->index(opposite));

      fi = ct.finite_faces_begin();
    }

    void traverse_triangulation(Face_handle f, int parent) {
      visited[f] = true;
      if(!ct.is_constrained(Edge(f,ct.cw(parent))) && !visited[f->neighbor(ct.cw(parent))]) {
	Face_handle child(f->neighbor(ct.cw(parent)));
	traverse_triangulation(child, child->index(f));
      } 
      if(!ct.is_constrained(Edge(f,ct.ccw(parent))) && !visited[f->neighbor(ct.ccw(parent))]) {
	Face_handle child(f->neighbor(ct.ccw(parent)));
	traverse_triangulation(child, child->index(f));
      } 
    } 
 
    template<typename Triangle_3>
    bool get_next_triangle(Triangle_3& tr) {
      while(fi != ct.finite_faces_end() && visited[fi] == false) ++fi;
      if(fi == ct.finite_faces_end()) return false;
      tr = Triangle_3(fi->vertex(0)->point(), fi->vertex(1)->point(), fi->vertex(2)->point());
      ++fi;
      return true;
    }

    bool same_orientation(typename Nef::Plane_3 p1) const {
      if(p1.a() != 0)
	return CGAL::sign(p1.a()) == CGAL::sign(supporting_plane.a());
      if(p1.b() != 0)
	return CGAL::sign(p1.b()) == CGAL::sign(supporting_plane.b());
      return CGAL::sign(p1.c()) == CGAL::sign(supporting_plane.c());
    }

    template<typename PIB, typename Index>
    void handle_triangles(PIB& pib, Index& VI) {
      while(fi != ct.finite_faces_end() && visited[fi] == false) ++fi;
      while(fi != ct.finite_faces_end()) {
	typename Nef::Plane_3 plane(fi->vertex(0)->point(),
		      fi->vertex(1)->point(),
		      fi->vertex(2)->point());
	pib.begin_facet();
	if(same_orientation(plane)) {
	  pib.add_vertex_to_facet(VI[ctv2v[fi->vertex(0)]]);
	  pib.add_vertex_to_facet(VI[ctv2v[fi->vertex(1)]]);
	  pib.add_vertex_to_facet(VI[ctv2v[fi->vertex(2)]]);
	} else {
	  pib.add_vertex_to_facet(VI[ctv2v[fi->vertex(0)]]);
	  pib.add_vertex_to_facet(VI[ctv2v[fi->vertex(2)]]);
	  pib.add_vertex_to_facet(VI[ctv2v[fi->vertex(1)]]);
	}
	pib.end_facet();
	do {
	  ++fi;
	} while(fi != ct.finite_faces_end() && visited[fi] == false);
      }
    }
};








template <class HDS, typename Kernel, typename Nef>
class Build_polyhedron : public CGAL::Modifier_base<HDS> {
public:
    bool error; // added for OpenSCAD
    class Visitor {
      typedef typename CGAL::Projection_traits_xy_3<Kernel>       XY;
      typedef typename CGAL::Projection_traits_yz_3<Kernel>       YZ;
      typedef typename CGAL::Projection_traits_xz_3<Kernel>       XZ;

      const CGAL::Object_index<typename Nef::Vertex_const_iterator>& VI;
      CGAL::Polyhedron_incremental_builder_3<HDS>& B;
      const typename Nef::SNC_const_decorator& D;
      
    public:
      bool error;//added for OpenSCAD
      Visitor(CGAL::Polyhedron_incremental_builder_3<HDS>& BB,
	      const typename Nef::SNC_const_decorator& sd,
	      CGAL::Object_index<typename Nef::Vertex_const_iterator>& vi) : VI(vi), B(BB), D(sd), error(false) {}

      void visit(typename Nef::Halffacet_const_handle opposite_facet) {

	CGAL_NEF_TRACEN("Build_polyhedron: visit facet " << opposite_facet->plane());
 
	CGAL_assertion(Nef::Infi_box::is_standard(opposite_facet->plane()));
	
	typename Nef::SHalfedge_const_handle se;
	typename Nef::Halffacet_cycle_const_iterator fc;
     	
	typename Nef::Halffacet_const_handle f = !opposite_facet->incident_volume()->mark() ? opposite_facet : opposite_facet->twin();

	typename Nef::SHalfedge_around_facet_const_circulator 
	  sfc1(f->facet_cycles_begin()), sfc2(sfc1);
	
	if(++f->facet_cycles_begin() != f->facet_cycles_end() ||
	   ++(++(++sfc1)) != sfc2) {
	  typename Nef::Vector_3 orth = f->plane().orthogonal_vector();
	  int c = CGAL::abs(orth[0]) > CGAL::abs(orth[1]) ? 0 : 1;
	  c = CGAL::abs(orth[2]) > CGAL::abs(orth[c]) ? 2 : c;

	try{	  // added for OpenSCAD
	  if(c == 0) {
	    Triangulation_handler2<YZ,Nef> th(f);
	    th.handle_triangles(B, VI);
	  } else if(c == 1) {
	    Triangulation_handler2<XZ,Nef> th(f);
	    th.handle_triangles(B, VI);
	  } else if(c == 2) {
	    Triangulation_handler2<XY,Nef> th(f);
	    th.handle_triangles(B, VI);
	  } else
	    CGAL_error_msg( "wrong value");
	 } catch (const CGAL::Failure_exception &e) { // added for OpenSCAD
	  PRINTB("WARNING: CGAL NefPolyhedron Triangulation failed: %s", e.what()); // added for OpenSCAD
	  this->error=true; //added for OpenSCAD
	 } // added for OpenSCAD
	} else {

	  B.begin_facet();
	  fc = f->facet_cycles_begin();
	  se = typename Nef::SHalfedge_const_handle(fc);
	  CGAL_assertion(se!=0);
	  typename Nef::SHalfedge_around_facet_const_circulator hc_start(se);
	  typename Nef::SHalfedge_around_facet_const_circulator hc_end(hc_start);
	  CGAL_For_all(hc_start,hc_end) {
	    CGAL_NEF_TRACEN("   add vertex " << hc_start->source()->center_vertex()->point());
	    B.add_vertex_to_facet(VI[hc_start->source()->center_vertex()]);
	  }
	  B.end_facet();
	}
      }

      void visit(typename Nef::SFace_const_handle) {}
      void visit(typename Nef::Halfedge_const_handle) {}
      void visit(typename Nef::Vertex_const_handle) {}
      void visit(typename Nef::SHalfedge_const_handle) {}
      void visit(typename Nef::SHalfloop_const_handle) {}
    };

  public:

    const typename Nef::SNC_const_decorator& scd;
    CGAL::Object_index<typename Nef::Vertex_const_iterator> VI;

    Build_polyhedron(const typename Nef::SNC_const_decorator& s) : error(false),
      scd(s), VI(s.vertices_begin(),s.vertices_end(),'V') {}
    
    void operator()( HDS& hds) {
      CGAL::Polyhedron_incremental_builder_3<HDS> B(hds, true);

      int skip_volumes;
      if(Nef::Infi_box::extended_kernel()) {
	B.begin_surface(scd.number_of_vertices()-8, 
			scd.number_of_facets()-6,
			scd.number_of_edges()-12);
	skip_volumes = 2;
      }
      else {
	B.begin_surface(scd.number_of_vertices(), 
			2*scd.number_of_vertices()-4,
			3*scd.number_of_vertices()-6);
	skip_volumes = 1;
      }
      
      int vertex_index = 0;
      typename Nef::Vertex_const_iterator v;
      CGAL_forall_vertices(v,scd) {
	if(Nef::Infi_box::is_standard(v->point())) {
	  VI[v]=vertex_index++;
	  B.add_vertex(v->point());
	}
      }     
      
      Visitor V(B,scd,VI);
      typename Nef::Volume_const_handle c;
      CGAL_forall_volumes(c,scd)
	if(skip_volumes-- <= 0) {
	  scd.visit_shell_objects(typename Nef:: SFace_const_handle(c->shells_begin()),V);
	}
     B.end_surface();
     this->error=B.error()||V.error; // added for OpenSCAD
     if (B.error()) B.rollback(); // added for OpenSCAD
    }

};

template <typename Kernel>
bool convert_to_Polyhedron( const CGAL::Nef_polyhedron_3<Kernel> &N, CGAL::Polyhedron_3<Kernel> &P )
{
	// several lines here added for OpenSCAD
	typedef typename CGAL::Nef_polyhedron_3<Kernel> Nef3;
	typedef typename CGAL::Polyhedron_3<Kernel> Polyhedron;
	typedef typename Polyhedron::HalfedgeDS HalfedgeDS;
	CGAL_precondition(N.is_simple());
	P.clear();
	Build_polyhedron<HalfedgeDS,Kernel,Nef3> bp(N);
	P.delegate(bp);
	return bp.error;
}





} //namespace nefworkaround
