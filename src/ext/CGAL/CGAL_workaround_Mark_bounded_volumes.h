// Copyright (c) 2005  Max-Planck-Institute Saarbruecken (Germany).
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
// $URL 
// $Id$ 
// 
//
// Author(s)     : Ralf Osbild <osbild@mpi-sb.mpg.de>

// Rewritten by Oskar Linde <oskar.linde@gmail.com>
// Now marks volumes recursively with alternating marks

#ifndef CGAL_NEF3_MARK_BOUNDED_VOLUMES_H
#define CGAL_NEF3_MARK_BOUNDED_VOLUMES_H

#include <set>
//#include <CGAL/Nef_polyhedron_3.h>
#include <CGAL/Modifier_base.h>
#include <CGAL/Nef_3/SNC_iteration.h>
#include <CGAL/assertions.h>

namespace CGAL {

template<typename Decorator, typename Mark>
class Volume_setter {

  typedef typename Decorator::Vertex_handle 
                                    Vertex_handle;
  typedef typename Decorator::Halfedge_handle 
                                    Halfedge_handle;
  typedef typename Decorator::Halffacet_handle 
                                    Halffacet_handle;
  typedef typename Decorator::SHalfedge_handle 
                                    SHalfedge_handle;
  typedef typename Decorator::SHalfloop_handle 
                                    SHalfloop_handle;
  typedef typename Decorator::SFace_handle 
                                    SFace_handle;
  Mark m;

public:
  std::set<typename Decorator::Volume_handle> opposite_volumes;

  Volume_setter(Mark m_in = true) : m(m_in) {}
  
  void visit(Vertex_handle ) {}
  void visit(Halfedge_handle ) {}
  void visit(Halffacet_handle hf) {
     opposite_volumes.insert(hf->twin()->incident_volume());
  }
  void visit(SHalfedge_handle ) {}
  void visit(SHalfloop_handle ) {}
  void visit(SFace_handle sf) {sf->mark() = m;}
};    

template<typename Nef_3>
class Mark_bounded_volumes : public Modifier_base<typename Nef_3::SNC_structure>
{  typedef typename Nef_3::SNC_structure         SNC_structure;
   typedef typename SNC_structure::SNC_decorator SNC_decorator;
   typedef typename SNC_structure::Infi_box      Infi_box;
   typedef typename Nef_3::SFace_handle          SFace_handle;
   typedef typename Nef_3::Volume_iterator       Volume_iterator;
   typedef typename Nef_3::Shell_entry_iterator 
                           Shell_entry_iterator;
   typedef typename Nef_3::Mark                  Mark;

   Mark flag;

public:
   Mark_bounded_volumes (Mark b=true) : flag(b) {}

   void mark_volume(SNC_decorator &D, typename SNC_structure::Volume_handle vol_it, bool mark, typename std::set<typename SNC_structure::Volume_handle> &visited) {
      Volume_setter<SNC_structure,Mark> vs(mark);
      vol_it->mark() = mark; // mark
      Shell_entry_iterator it;
      CGAL_forall_shells_of(it,vol_it) {
	 D.visit_shell_objects(SFace_handle(it),vs);
      }
      for(typename std::set<typename SNC_structure::Volume_handle>::iterator i =
	  vs.opposite_volumes.begin();
	  i != vs.opposite_volumes.end(); ++i) {
	 if (!visited.count(*i)) {
	    visited.insert(*i);
	    mark_volume(D, *i, !mark, visited);
	 }
      }
   }

   void operator()(SNC_structure &snc) override
   {  // mark bounded volumes
      Volume_iterator vol_it = snc.volumes_begin();
      CGAL_assertion ( vol_it != snc.volumes_end() );
      if ( Infi_box::extended_kernel() ) ++vol_it; // skip Infi_box
      CGAL_assertion ( vol_it != snc.volumes_end() );

      typename std::set<typename SNC_structure::Volume_handle> visited;
      // mark the unbounded volume and recursively visit the other volumes
      SNC_decorator D(snc);
      visited.insert(vol_it);
      mark_volume(D, vol_it, !flag, visited);
   }
};

} //namespace CGAL
#endif // CGAL_NEF3_MARK_BOUNDED_VOLUMES_H
