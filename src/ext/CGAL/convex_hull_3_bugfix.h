/*
	This file contains a bugfix against CGAL-4.5.1, see:
	http://cgal-discuss.949826.n4.nabble.com/Epick-convex-hull-3-assertion-td4660264.html
*/

// Copyright (c) 2001,2011  Max-Planck-Institute Saarbruecken (Germany).
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
// $URL$
// $Id$
// 
//
// Author(s)     : Susan Hert <hert@mpi-sb.mpg.de>
//               : Amol Prakash <prakash@mpi-sb.mpg.de>
//               : Andreas Fabri

#ifndef CGAL_CONVEX_HULL_3_H
#define CGAL_CONVEX_HULL_3_H
#include <CGAL/basic.h>
#include <CGAL/algorithm.h> 
#include <CGAL/convex_hull_2.h>
#include <CGAL/Polyhedron_incremental_builder_3.h>
#include <CGAL/Projection_traits_xy_3.h>
#include <CGAL/Projection_traits_xz_3.h>
#include <CGAL/Projection_traits_yz_3.h>
#include <CGAL/Convex_hull_traits_3.h>
#include <CGAL/Convex_hull_2/ch_assertions.h>
#include <CGAL/Triangulation_data_structure_2.h>
#include <CGAL/Triangulation_vertex_base_with_info_2.h>
#include <CGAL/Cartesian_converter.h>
#include <CGAL/Simple_cartesian.h>
#include <iostream>
#include <algorithm>
#include <utility>
#include <list>
#include <map>
#include <vector>
#include <boost/bind.hpp>
#include <boost/next_prior.hpp>
#include <boost/type_traits/is_floating_point.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/mpl/has_xxx.hpp>
#include <CGAL/internal/Exact_type_selector.h>


#ifndef CGAL_CH_NO_POSTCONDITIONS
#include <CGAL/convexity_check_3.h>
#endif // CGAL_CH_NO_POSTCONDITIONS


namespace CGAL {

namespace internal{  namespace Convex_hull_3{

//struct to select the default traits class for computing convex hull
template< class Point_3,
          class Is_floating_point=typename boost::is_floating_point<typename Kernel_traits<Point_3>::Kernel::FT>::type,
          class Has_filtered_predicates_tag=typename Kernel_traits<Point_3>::Kernel::Has_filtered_predicates_tag >
struct Default_traits_for_Chull_3{
  typedef typename Kernel_traits<Point_3>::Kernel type;
};

//FT is a floating point type and Kernel is a filtered kernel
template <class Point_3>
struct Default_traits_for_Chull_3<Point_3,boost::true_type,Tag_true>{
  typedef Convex_hull_traits_3< typename Kernel_traits<Point_3>::Kernel > type;
};

template <class Traits>
struct Default_polyhedron_for_Chull_3{
  typedef CGAL::Polyhedron_3<Traits> type;
};

template <class K>
struct Default_polyhedron_for_Chull_3<Convex_hull_traits_3<K>>{
  typedef typename  Convex_hull_traits_3<K>::Polyhedron_3 type;
};
 
//utility class to select the right version of internal predicate Is_on_positive_side_of_plane_3
template <class Traits,
          class Is_floating_point=
            typename boost::is_floating_point<typename Kernel_traits<typename Traits::Point_3>::Kernel::FT>::type,
          class Has_filtered_predicates_tag=typename Kernel_traits<typename Traits::Point_3>::Kernel::Has_filtered_predicates_tag,
          class Has_cartesian_tag=typename Kernel_traits<typename Traits::Point_3>::Kernel::Kernel_tag,
          class Has_classical_point_type =
              typename boost::is_same<
                typename Kernel_traits<typename Traits::Point_3>::Kernel::Point_3,
                typename Traits::Point_3  >::type
         >
struct Use_advanced_filtering{
  typedef CGAL::Tag_false type;
};

template <class Traits>
struct Use_advanced_filtering<Traits,boost::true_type,Tag_true,Cartesian_tag,boost::true_type>{
  typedef typename Kernel_traits<typename Traits::Point_3>::Kernel K;
  typedef CGAL::Boolean_tag<K::Has_static_filters> type;
};

//Predicates internally used
template <class Traits,class Tag_use_advanced_filtering=typename Use_advanced_filtering<Traits>::type >
class Is_on_positive_side_of_plane_3{
  typedef typename Traits::Point_3 Point_3;
  typename Traits::Plane_3 plane;
  typename Traits::Has_on_positive_side_3 has_on_positive_side;
public:
  typedef Protect_FPU_rounding<false> Protector;

  Is_on_positive_side_of_plane_3(const Traits& traits,const Point_3& p,const Point_3& q,const Point_3& r)
  :plane(traits.construct_plane_3_object()(p,q,r)),has_on_positive_side(traits.has_on_positive_side_3_object()) {}
    
  bool operator() (const Point_3& s) const 
  {
    return has_on_positive_side(plane,s);
  }
};
  

//This predicate uses copy of the code from the statically filtered version of
//Orientation_3. The rational is that the plane is a member of the functor
//so optimization are done to avoid doing several time operations on the plane.
//The main operator() first tries the static version of the predicate, then uses
//interval arithmetic (the protector must be created before using this predicate)
//and in case of failure, exact arithmetic is used.
template <class Kernel>
class Is_on_positive_side_of_plane_3<Convex_hull_traits_3<Kernel>,Tag_true>{
  typedef Simple_cartesian<CGAL::internal::Exact_field_selector<double>::Type>         PK;
  typedef Simple_cartesian<Interval_nt_advanced >                               CK;  
  typedef Convex_hull_traits_3<Kernel>                                          Traits;
  typedef typename Traits::Point_3                                              Point_3;
  
  Cartesian_converter<Kernel,CK>                        to_CK;
  Cartesian_converter<Kernel,PK>                        to_PK;

  const Point_3& p,q,r;
  mutable typename CK::Plane_3* ck_plane;
  mutable typename PK::Plane_3* pk_plane;
 
  double m10,m20,m21,Maxx,Maxy,Maxz;
  
  static const int STATIC_FILTER_FAILURE = 555;
  
  //this function is a made from the statically filtered version of Orientation_3
  int static_filtered(double psx,double psy, double psz) const{

    // Then semi-static filter.
    double apsx = CGAL::abs(psx);
    double apsy = CGAL::abs(psy);
    double apsz = CGAL::abs(psz);

    double maxx = (Maxx < apsx)? apsx : Maxx;
    double maxy = (Maxy < apsy)? apsy : Maxy;
    double maxz = (Maxz < apsz)? apsz : Maxz;

    double det =  psx*m10 - m20*psy + m21*psz;
    
    // Sort maxx < maxy < maxz.
    if (maxx > maxz)
        std::swap(maxx, maxz);
    if (maxy > maxz)
        std::swap(maxy, maxz);
    else if (maxy < maxx)
        std::swap(maxx, maxy);

    // Protect against underflow in the computation of eps.
    if (maxx < 1e-97) /* cbrt(min_double/eps) */ {
      if (maxx == 0)
        return 0;
    }
    // Protect against overflow in the computation of det.
    else if (maxz < 1e102) /* cbrt(max_double [hadamard]/4) */ {
      double eps = 5.1107127829973299e-15 * maxx * maxy * maxz;
      if (det > eps)  return 1;
      if (det < -eps) return -1;
    }
    return STATIC_FILTER_FAILURE;
  }
  
public:
  typedef typename Interval_nt_advanced::Protector           Protector;

  Is_on_positive_side_of_plane_3(const Traits&,const Point_3& p_,const Point_3& q_,const Point_3& r_)
  :p(p_),q(q_),r(r_),ck_plane(NULL),pk_plane(NULL)
  {
    double pqx = q.x() - p.x();
    double pqy = q.y() - p.y();
    double pqz = q.z() - p.z();
    double prx = r.x() - p.x();
    double pry = r.y() - p.y();
    double prz = r.z() - p.z();   


    m10 = pqy*prz - pry*pqz;
    m20 = pqx*prz - prx*pqz;
    m21 = pqx*pry - prx*pqy;
    
    double aprx = CGAL::abs(prx);
    double apry = CGAL::abs(pry);
    double aprz = CGAL::abs(prz);

    Maxx = CGAL::abs(pqx);
    if (Maxx < aprx) Maxx = aprx;
    Maxy = CGAL::abs(pqy);
    if (Maxy < apry) Maxy = apry;
    Maxz = CGAL::abs(pqz);
    if (Maxz < aprz) Maxz = aprz;
  }

  ~Is_on_positive_side_of_plane_3(){
    if (ck_plane!=NULL) delete ck_plane;
    if (pk_plane!=NULL) delete pk_plane;
  }
  
  bool operator() (const Point_3& s) const 
  {
    double psx = s.x() - p.x();
    double psy = s.y() - p.y();
    double psz = s.z() - p.z(); 
    
    int static_res = static_filtered(psx,psy,psz);
    if (static_res != STATIC_FILTER_FAILURE)
      return static_res == 1;
    
    try{
      if (ck_plane==NULL)
        ck_plane=new typename CK::Plane_3(to_CK(p),to_CK(q),to_CK(r));
      return ck_plane->has_on_positive_side(to_CK(s));
    }
    catch (Uncertain_conversion_exception){
      if (pk_plane==NULL)
        pk_plane=new typename PK::Plane_3(to_PK(p),to_PK(q),to_PK(r));
      return pk_plane->has_on_positive_side(to_PK(s));
    }
  }
};


template<class HDS, class ForwardIterator>
class Build_coplanar_poly : public Modifier_base<HDS> {
 public:
  Build_coplanar_poly(ForwardIterator i, ForwardIterator j) 
    {
      start = i;
      end = j;
    }
  void operator()( HDS& hds) {
    Polyhedron_incremental_builder_3<HDS> B(hds,true);
    ForwardIterator iter = start;
    int count = 0;
    while (iter != end)
      {
	count++;
	iter++;
      }
    B.begin_surface(count, 1, 2*count);
    iter = start;
    while (iter != end)
      {
	B.add_vertex(*iter);
	iter++;
      }
    iter = start;
    B.begin_facet();
    int p = 0;
    while (p < count)
      {
	B.add_vertex_to_facet(p);
	p++;
      }
    B.end_facet();
    B.end_surface();
  }
 private:
  ForwardIterator start;
  ForwardIterator end;    
};


namespace internal { namespace Convex_hull_3{

BOOST_MPL_HAS_XXX_TRAIT_NAMED_DEF(Traits_has_typedef_Traits_xy_3,Traits_xy_3,false)
BOOST_MPL_HAS_XXX_TRAIT_NAMED_DEF(Traits_has_typedef_Traits_yz_3,Traits_xy_3,false)
BOOST_MPL_HAS_XXX_TRAIT_NAMED_DEF(Traits_has_typedef_Traits_xz_3,Traits_xy_3,false)

template <class T,bool has_projection_traits=
  Traits_has_typedef_Traits_xy_3<T>::value &&
  Traits_has_typedef_Traits_yz_3<T>::value &&
  Traits_has_typedef_Traits_xz_3<T>::value
>
struct Projection_traits{
  typedef typename Kernel_traits<typename T::Point_3>::Kernel K;
  typedef CGAL::Projection_traits_xy_3<K> Traits_xy_3;
  typedef CGAL::Projection_traits_yz_3<K> Traits_yz_3;
  typedef CGAL::Projection_traits_xz_3<K> Traits_xz_3;
};

template <class T>
struct Projection_traits<T,true>{
  typedef typename T::Traits_xy_3 Traits_xy_3;
  typedef typename T::Traits_yz_3 Traits_yz_3;
  typedef typename T::Traits_xz_3 Traits_xz_3;
};

} } //end of namespace internal::Convex_hull_3

template <class InputIterator, class Point_3, class Polyhedron_3, class Traits>
void coplanar_3_hull(InputIterator first, InputIterator beyond,
                     const Point_3& p1, const Point_3& p2, const Point_3& p3, 
                     Polyhedron_3& P, const Traits& /* traits */)
{
  typedef typename internal::Convex_hull_3::Projection_traits<Traits> PTraits;
  typedef typename PTraits::Traits_xy_3 Traits_xy_3;
  typedef typename PTraits::Traits_yz_3 Traits_yz_3;
  typedef typename PTraits::Traits_xz_3 Traits_xz_3;

  std::list<Point_3> CH_2;
  typedef typename std::list<Point_3>::iterator  CH_2_iterator;
 
  Traits_xy_3 traits_xy;
  typename Traits_xy_3::Left_turn_2 left_turn_in_xy = traits_xy.left_turn_2_object();
  if ( left_turn_in_xy(p1,p2,p3) || left_turn_in_xy(p2,p1,p3) )
     convex_hull_points_2( first, beyond,
                           std::back_inserter(CH_2),
                           traits_xy );
  else{
    Traits_yz_3 traits_yz;
    typename Traits_yz_3::Left_turn_2 left_turn_in_yz = traits_yz.left_turn_2_object();
    if ( left_turn_in_yz(p1,p2,p3) || left_turn_in_yz(p2,p1,p3) )
       convex_hull_points_2( first, beyond,
                             std::back_inserter(CH_2),
                             traits_yz );
    else{
      Traits_xz_3 traits_xz;
      typename Traits_xz_3::Left_turn_2 left_turn_in_xz = traits_xz.left_turn_2_object();
      CGAL_assertion( left_turn_in_xz(p1,p2,p3) || left_turn_in_xz(p2,p1,p3) );
         convex_hull_points_2( first, beyond,
                               std::back_inserter(CH_2),
                               traits_xz );
    }
  }

  typedef typename Polyhedron_3::Halfedge_data_structure HDS;

  Build_coplanar_poly<HDS,CH_2_iterator> poly(CH_2.begin(),CH_2.end());
  P.delegate(poly);
}


//
// visible is the set of facets visible from point  and reachable from
// start_facet.
//
template <class TDS_2, class Traits>
void
find_visible_set(TDS_2& tds, 
                 const typename Traits::Point_3& point, 
                 typename TDS_2::Face_handle start,
                 std::list<typename TDS_2::Face_handle>& visible,
                 std::map<typename TDS_2::Vertex_handle, typename TDS_2::Edge>& outside,
                 const Traits& traits)
{
   typedef typename Traits::Plane_3                   Plane_3;
   typedef typename TDS_2::Face_handle Face_handle;
   typedef typename TDS_2::Vertex_handle Vertex_handle;
   typename Traits::Has_on_positive_side_3 has_on_positive_side =
            traits.has_on_positive_side_3_object();

   std::vector<Vertex_handle> vertices;
   vertices.reserve(10);
   int VISITED=1, BORDER=2;
   visible.clear();
   typename std::list<Face_handle>::iterator  vis_it;
   visible.push_back(start);
   start->info() = VISITED;
   vertices.push_back(start->vertex(0));
   vertices.push_back(start->vertex(1));
   vertices.push_back(start->vertex(2));
   start->vertex(0)->info() = start->vertex(1)->info() = start->vertex(2)->info() = VISITED;
 
   for (vis_it = visible.begin(); vis_it != visible.end(); vis_it++)
   {
      // check all the neighbors of the current face to see if they have 
      // already been visited or not and if not whether they are visible 
      // or not.

      for(int i=0; i < 3; i++) {
        // the facet on the other side of the current halfedge
        Face_handle f = (*vis_it)->neighbor(i);
        // if haven't already seen this facet
        if (f->info() == 0) {
          f->info() = VISITED;
          Plane_3 plane(f->vertex(0)->point(),f->vertex(1)->point(),f->vertex(2)->point());
          int ind = f->index(*vis_it);
          if ( has_on_positive_side(plane, point) ){  // is visible
            visible.push_back(f);
            Vertex_handle vh = f->vertex(ind);
            if(vh->info() == 0){ vertices.push_back(vh); vh->info() = VISITED;}
          } else {
            f->info() = BORDER;
            f->vertex(TDS_2::cw(ind))->info() = BORDER;            
            f->vertex(TDS_2::ccw(ind))->info() = BORDER;
            outside.insert(std::make_pair(f->vertex(TDS_2::cw(ind)),
                                          typename TDS_2::Edge(f,ind)));
          }
        } else if(f->info() == BORDER) {
          int ind = f->index(*vis_it);
          f->vertex(TDS_2::cw(ind))->info() = BORDER;            
          f->vertex(TDS_2::ccw(ind))->info() = BORDER;
          outside.insert(std::make_pair(f->vertex(TDS_2::cw(ind)),
                                        typename TDS_2::Edge(f,ind)));
        }
      }
   }
 
   for(typename std::vector<Vertex_handle>::iterator vit =  vertices.begin();
       vit != vertices.end();
       ++vit){
     if((*vit)->info() != BORDER){
       tds.delete_vertex(*vit);
     } else {
       (*vit)->info() = 0;
     }
   }

}

// using a third template parameter for the point instead of getting it from
// the traits class as it should be is required by M$VC6
template <class Face_handle, class Traits, class Point>
typename std::list<Point>::iterator
farthest_outside_point(Face_handle f, std::list<Point>& outside_set,
                       const Traits& traits)
{

   typedef typename std::list<Point>::iterator Outside_set_iterator;
   CGAL_ch_assertion(!outside_set.empty());

   typename Traits::Plane_3 plane(f->vertex(0)->point(),f->vertex(1)->point(),f->vertex(2)->point());

   typename Traits::Less_signed_distance_to_plane_3 less_dist_to_plane =
            traits.less_signed_distance_to_plane_3_object();
   Outside_set_iterator farthest_it =
          std::max_element(outside_set.begin(),
                           outside_set.end(), 
                           boost::bind(less_dist_to_plane, plane, _1, _2));
   return farthest_it;
}

template <class Face_handle, class Traits, class Point>
void     
partition_outside_sets(const std::list<Face_handle>& new_facets,
                       std::list<Point>& vis_outside_set, 
                       std::list<Face_handle>& pending_facets,
                       const Traits& traits)
{
  typename std::list<Face_handle>::const_iterator        f_list_it;
  typename std::list<Point>::iterator  point_it, to_splice;
   
  // walk through all the new facets and check each unassigned outside point
  // to see if it belongs to the outside set of this new facet.
  for (f_list_it = new_facets.begin(); (f_list_it != new_facets.end()) && (! vis_outside_set.empty());
        ++f_list_it)
  {
    Face_handle f = *f_list_it;
    Is_on_positive_side_of_plane_3<Traits> is_on_positive_side(
      traits,f->vertex(0)->point(),f->vertex(1)->point(),f->vertex(2)->point());
    std::list<Point>& point_list = f->points;

    for (point_it = vis_outside_set.begin();point_it != vis_outside_set.end();){
      if( is_on_positive_side(*point_it) ) {
        to_splice = point_it;
        ++point_it;
        point_list.splice(point_list.end(), vis_outside_set, to_splice);
      } else {
         ++point_it;
      }
    }
    if(! point_list.empty()){
      pending_facets.push_back(f);
      f->it = boost::prior(pending_facets.end());
    } else {
      f->it = pending_facets.end();
    }
  }
   
   
   for (; f_list_it != new_facets.end();++f_list_it)
    (*f_list_it)->it = pending_facets.end();
}



template <class TDS_2, class Traits>
void
ch_quickhull_3_scan(TDS_2& tds,
                    std::list<typename TDS_2::Face_handle>& pending_facets,
                    const Traits& traits)
{
  typedef typename TDS_2::Edge                            Edge;
  typedef typename TDS_2::Face_handle                     Face_handle;
  typedef typename TDS_2::Vertex_handle                   Vertex_handle;
  typedef typename Traits::Point_3			  Point_3;
  typedef std::list<Point_3>                              Outside_set;
  typedef typename std::list<Point_3>::iterator           Outside_set_iterator;
  typedef std::map<typename TDS_2::Vertex_handle, typename TDS_2::Edge> Border_edges;

  std::list<Face_handle>                     visible_set;
  typename std::list<Face_handle>::iterator  vis_set_it;
  Outside_set                                vis_outside_set;
  Border_edges                               border;

  while (!pending_facets.empty())
  {
     vis_outside_set.clear();

     Face_handle f_handle = pending_facets.front();

     Outside_set_iterator farthest_pt_it = farthest_outside_point(f_handle, f_handle->points, traits);
     Point_3 farthest_pt = *farthest_pt_it;
     f_handle->points.erase(farthest_pt_it);
     find_visible_set(tds, farthest_pt, f_handle, visible_set, border, traits);

     // for each visible facet
     for (vis_set_it = visible_set.begin(); vis_set_it != visible_set.end();
          vis_set_it++)
     {
       
        //   add its outside set to the global outside set list
       std::list<Point_3>& point_list = (*vis_set_it)->points;
       if(! point_list.empty()){
         vis_outside_set.splice(vis_outside_set.end(), point_list, point_list.begin(), point_list.end());
       }

       if((*vis_set_it)->it != pending_facets.end()){
         pending_facets.erase((*vis_set_it)->it);
       }
       (*vis_set_it)->info() = 0;
     }

     std::vector<Edge> edges;
     edges.reserve(border.size());
     typename Border_edges::iterator it = border.begin();
     Edge e = it->second;
     e.first->info() = 0; 
     edges.push_back(e);
     border.erase(it);
     while(! border.empty()){
       it = border.find(e.first->vertex(TDS_2::ccw(e.second)));
       assert(it != border.end());
       e = it->second;
       e.first->info() = 0; 
       edges.push_back(e);
       border.erase(it);
     }

     // If we want to reuse the faces we must only pass |edges| many, and call delete_face for the others.
     // Also create facets if necessary
     std::ptrdiff_t diff = visible_set.size() - edges.size();
     if(diff < 0){
       for(int i = 0; i<-diff;i++){
         visible_set.push_back(tds.create_face());
       }
     } else {
       for(int i = 0; i<diff;i++){
         tds.delete_face(visible_set.back());
         visible_set.pop_back();
       }
     }
     Vertex_handle vh = tds.star_hole(edges.begin(), edges.end(), visible_set.begin(), visible_set.end());
     vh->point() = farthest_pt;
     vh->info() = 0;     
  
     // now partition the set of outside set points among the new facets.
   
     partition_outside_sets(visible_set, vis_outside_set, 
                            pending_facets, traits);

  }
}

template <class TDS_2, class Traits>
void non_coplanar_quickhull_3(std::list<typename Traits::Point_3>& points,
                              TDS_2& tds, const Traits& traits)
{
  typedef typename Traits::Point_3                        Point_3;

  typedef typename TDS_2::Face_handle                     Face_handle;
  typedef typename TDS_2::Face_iterator                     Face_iterator;
  typedef typename std::list<Point_3>::iterator           P3_iterator;

  std::list<Face_handle> pending_facets;

  typename Is_on_positive_side_of_plane_3<Traits>::Protector p;
  
  // for each facet, look at each unassigned point and decide if it belongs
  // to the outside set of this facet.
  for(Face_iterator fit = tds.faces_begin(); fit != tds.faces_end(); ++fit){
    Is_on_positive_side_of_plane_3<Traits> is_on_positive_side(
      traits,fit->vertex(0)->point(),fit->vertex(1)->point(),fit->vertex(2)->point() );
    for (P3_iterator point_it = points.begin() ; point_it != points.end(); )
    {
      if( is_on_positive_side(*point_it) ) {
        P3_iterator to_splice = point_it;
        ++point_it;
        fit->points.splice(fit->points.end(), points, to_splice);
      } else {
       ++point_it;
      }
    }
  }
  // add all the facets with non-empty outside sets to the set of facets for
  // further consideration
  for(Face_iterator fit = tds.faces_begin(); fit != tds.faces_end(); ++fit){
    if (! fit->points.empty()){
      pending_facets.push_back(fit);
      fit->it = boost::prior(pending_facets.end());
        } else {
      fit->it =  pending_facets.end();
    }
  }


  ch_quickhull_3_scan(tds, pending_facets, traits);

  //std::cout << "|V(tds)| = " << tds.number_of_vertices() << std::endl;
//  CGAL_ch_expensive_postcondition(all_points_inside(points.begin(),
//                                                    points.end(),P,traits));
//  CGAL_ch_postcondition(is_strongly_convex_3(P, traits));
}


namespace internal{
  
template <class HDS,class TDS>
class Build_convex_hull_from_TDS_2 : public CGAL::Modifier_base<HDS> {
  typedef std::map<typename TDS::Vertex_handle,unsigned> Vertex_map;
  
  const TDS& t;
  template <class Builder>
  static unsigned get_vertex_index( Vertex_map& vertex_map,
                                    typename TDS::Vertex_handle vh,
                                    Builder& builder,
                                    unsigned& vindex)
  {
    std::pair<typename Vertex_map::iterator,bool>
      res=vertex_map.insert(std::make_pair(vh,vindex));
    if (res.second){
      builder.add_vertex(vh->point());
      ++vindex;
    }
    return res.first->second;
  }
  
public:
  Build_convex_hull_from_TDS_2(const TDS& t_):t(t_) 
  {
    CGAL_assertion(t.dimension()==2);
  }
  void operator()( HDS& hds) {
    // Postcondition: `hds' is a valid polyhedral surface.
    
    CGAL::Polyhedron_incremental_builder_3<HDS> B( hds, true);
    Vertex_map vertex_map;
    //start the surface
    B.begin_surface( t.number_of_vertices(), t.number_of_faces());
    unsigned vindex=0;
    for (typename TDS::Face_iterator it=t.faces_begin();it!=t.faces_end();++it)
    {
      unsigned i0=get_vertex_index(vertex_map,it->vertex(0),B,vindex);
      unsigned i1=get_vertex_index(vertex_map,it->vertex(1),B,vindex);
      unsigned i2=get_vertex_index(vertex_map,it->vertex(2),B,vindex);
      B.begin_facet();
      B.add_vertex_to_facet( i0 );
      B.add_vertex_to_facet( i1 );
      B.add_vertex_to_facet( i2 );
      B.end_facet();      
    }
    B.end_surface();
  }
};
  
} //namespace internal

template <class InputIterator, class Polyhedron_3, class Traits>
void
ch_quickhull_polyhedron_3(std::list<typename Traits::Point_3>& points,
                          InputIterator point1_it, InputIterator point2_it,
                          InputIterator point3_it, Polyhedron_3& P,
                          const Traits& traits)
{
  typedef typename Traits::Point_3	  		  Point_3;  
  typedef typename Traits::Plane_3		      	  Plane_3;
  typedef typename std::list<Point_3>::iterator           P3_iterator;

  typedef Triangulation_data_structure_2<
    Triangulation_vertex_base_with_info_2<int, GT3_for_CH3<Traits>>,
    Convex_hull_face_base_2<int, Traits>>                           Tds;
  typedef typename Tds::Vertex_handle                     Vertex_handle;
  typedef typename Tds::Face_handle                     Face_handle;

  // found three points that are not collinear, so construct the plane defined
  // by these points and then find a point that has maximum distance from this
  // plane.   
  typename Traits::Construct_plane_3 construct_plane =
         traits.construct_plane_3_object();
  Plane_3 plane = construct_plane(*point3_it, *point2_it, *point1_it);
  typedef typename Traits::Less_signed_distance_to_plane_3      Dist_compare; 
  Dist_compare compare_dist = traits.less_signed_distance_to_plane_3_object();
  
  typename Traits::Coplanar_3  coplanar = traits.coplanar_3_object(); 
  // find both min and max here since using signed distance.  If all points
  // are on the negative side of the plane, the max element will be on the
  // plane.
  std::pair<P3_iterator, P3_iterator> min_max;
  min_max = CGAL::min_max_element(points.begin(), points.end(), 
                                  boost::bind(compare_dist, plane, _1, _2),
                                  boost::bind(compare_dist, plane, _1, _2));
  P3_iterator max_it;
  if (coplanar(*point1_it, *point2_it, *point3_it, *min_max.second))
  {
     max_it = min_max.first;
     // want the orientation of the points defining the plane to be positive
     // so have to reorder these points if all points were on negative side
     // of plane
     std::swap(*point1_it, *point3_it);
  }
  else
     max_it = min_max.second;

  // if the maximum distance point is on the plane then all are coplanar
  if (coplanar(*point1_it, *point2_it, *point3_it, *max_it)) {
     coplanar_3_hull(points.begin(), points.end(), *point1_it, *point2_it, *point3_it, P, traits);
  } else {  
    Tds tds;
    Vertex_handle v0 = tds.create_vertex(); v0->set_point(*point1_it);
    Vertex_handle v1 = tds.create_vertex(); v1->set_point(*point2_it);
    Vertex_handle v2 = tds.create_vertex(); v2->set_point(*point3_it);
    Vertex_handle v3 = tds.create_vertex(); v3->set_point(*max_it);

    v0->info() = v1->info() = v2->info() = v3->info() = 0;
    Face_handle f0 = tds.create_face(v0,v1,v2);
    Face_handle f1 = tds.create_face(v3,v1,v0);
    Face_handle f2 = tds.create_face(v3,v2,v1);
    Face_handle f3 = tds.create_face(v3,v0,v2);
    tds.set_dimension(2);
    f0->set_neighbors(f2, f3, f1);
    f1->set_neighbors(f0, f3, f2);
    f2->set_neighbors(f0, f1, f3);
    f3->set_neighbors(f0, f2, f1);

    points.erase(point1_it);
    points.erase(point2_it);
    points.erase(point3_it);
    points.erase(max_it);
    if (!points.empty()){
      non_coplanar_quickhull_3(points, tds, traits);
      internal::Build_convex_hull_from_TDS_2<typename Polyhedron_3::HalfedgeDS,Tds> builder(tds);
      P.delegate(builder);
    }
    else
      P.make_tetrahedron(v0->point(),v1->point(),v2->point(),v3->point());
  }
  
}

} } //namespace internal::Convex_hull_3

template <class InputIterator, class Traits>
void
convex_hull_3(InputIterator first, InputIterator beyond, 
              Object& ch_object, const Traits& traits)
{  
  typedef typename Traits::Point_3	  		  Point_3;  
  typedef std::list<Point_3>                              Point_3_list;
  typedef typename Point_3_list::iterator                 P3_iterator;
  typedef std::pair<P3_iterator,P3_iterator>              P3_iterator_pair;

  if (first == beyond)    // No point
    return;

  // If the first and last point are equal the collinearity test some lines below will always be true.
  Point_3_list points(first, beyond);
  std::size_t size = points.size();
  while((size > 1) && (points.front() == points.back())){
    points.pop_back();
    --size;
  }

  if ( size == 1 )                // 1 point 
  {
      ch_object = make_object(*points.begin());
      return;
  }
  else if ( size == 2 )           // 2 points 
  {
      typedef typename Traits::Segment_3                 Segment_3;  
      typename Traits::Construct_segment_3 construct_segment =
             traits.construct_segment_3_object();
      Segment_3 seg = construct_segment(*points.begin(), *(++points.begin()));
      ch_object = make_object(seg);
      return;
  }
  else if ( size == 3 )           // 3 points 
  {
      typedef typename Traits::Triangle_3                Triangle_3;  
      typename Traits::Construct_triangle_3 construct_triangle =
             traits.construct_triangle_3_object();
      Triangle_3 tri = construct_triangle(*(points.begin()), 
                                          *(++points.begin()),
                                          *(--points.end()));
      ch_object = make_object(tri);
      return;
  }

  // at least 4 points 
  typename Traits::Collinear_3 collinear = traits.collinear_3_object();
  
  P3_iterator point1_it = points.begin();
  P3_iterator point2_it = points.begin();
  point2_it++;
  P3_iterator point3_it = points.end();
  point3_it--;

  // find three that are not collinear
  while (point2_it != points.end() && 
         collinear(*point1_it,*point2_it,*point3_it))
    point2_it++;
  

  // all are collinear, so the answer is a segment
  if (point2_it == points.end())
  {
     typedef typename Traits::Less_distance_to_point_3      Less_dist; 

     Less_dist less_dist = traits.less_distance_to_point_3_object();
     P3_iterator_pair endpoints = 
      min_max_element(points.begin(), points.end(), 
                      boost::bind(less_dist, *points.begin(), _1, _2), 
                      boost::bind(less_dist, *points.begin(), _1, _2));

     typename Traits::Construct_segment_3 construct_segment =
            traits.construct_segment_3_object();
     typedef typename Traits::Segment_3                 Segment_3;  

     Segment_3 seg = construct_segment(*endpoints.first, *endpoints.second);
     ch_object = make_object(seg);
     return;
  }

  // result will be a polyhedron
  typename internal::Convex_hull_3::Default_polyhedron_for_Chull_3<Traits>::type P;

  P3_iterator minx, maxx, miny, it;
  minx = maxx = miny = it = points.begin();
  ++it;
  for(; it != points.end(); ++it){
    if(it->x() < minx->x()) minx = it;
    if(it->x() > maxx->x()) maxx = it;
    if(it->y() < miny->y()) miny = it;
  }
  if(! collinear(*minx, *maxx, *miny) ){  
    internal::Convex_hull_3::ch_quickhull_polyhedron_3(points, minx, maxx, miny, P, traits);
  } else {
    internal::Convex_hull_3::ch_quickhull_polyhedron_3(points, point1_it, point2_it, point3_it, P, traits);
  }
  CGAL_assertion(P.size_of_vertices()>=3);
  if (boost::next(P.vertices_begin(),3) == P.vertices_end()){
    typedef typename Traits::Triangle_3                Triangle_3;
    typename Traits::Construct_triangle_3 construct_triangle =
           traits.construct_triangle_3_object();
    Triangle_3 tri = construct_triangle(P.halfedges_begin()->vertex()->point(), 
                                        P.halfedges_begin()->next()->vertex()->point(),
                                        P.halfedges_begin()->opposite()->vertex()->point());
    ch_object = make_object(tri);
  }
  else
    ch_object = make_object(P);
}


template <class InputIterator>
void convex_hull_3(InputIterator first, InputIterator beyond, 
		   Object& ch_object)
{
   typedef typename std::iterator_traits<InputIterator>::value_type Point_3;
   typedef typename internal::Convex_hull_3::Default_traits_for_Chull_3<Point_3>::type Traits;
   convex_hull_3(first, beyond, ch_object, Traits());
}



template <class InputIterator, class Polyhedron_3, class Traits>
void convex_hull_3(InputIterator first, InputIterator beyond,
                   Polyhedron_3& polyhedron,  const Traits& traits)
{
  typedef typename Traits::Point_3                Point_3;  
  typedef std::list<Point_3>                      Point_3_list;
  typedef typename Point_3_list::iterator         P3_iterator;

  Point_3_list points(first, beyond);
  CGAL_ch_precondition(points.size() > 3);

  // at least 4 points 
  typename Traits::Collinear_3 collinear = traits.collinear_3_object();
  typename Traits::Equal_3 equal = traits.equal_3_object();

  P3_iterator point1_it = points.begin();
  P3_iterator point2_it = points.begin();
  point2_it++;

  // find three that are not collinear
  while (point2_it != points.end() && equal(*point1_it,*point2_it))
    ++point2_it;

  CGAL_ch_precondition_msg(point2_it != points.end(), 
        "All points are equal; cannot construct polyhedron.");
  
  P3_iterator point3_it = point2_it;
  ++point3_it;
  
  CGAL_ch_precondition_msg(point3_it != points.end(), 
        "Only two points with different coordinates; cannot construct polyhedron.");
  
  while (point3_it != points.end() && collinear(*point1_it,*point2_it,*point3_it))
    ++point3_it;
  
  CGAL_ch_precondition_msg(point3_it != points.end(), 
        "All points are collinear; cannot construct polyhedron.");
  
  polyhedron.clear();
  // result will be a polyhedron
  internal::Convex_hull_3::ch_quickhull_polyhedron_3(points, point1_it, point2_it, point3_it,
                                                     polyhedron, traits);

}


template <class InputIterator, class Polyhedron_3>
void convex_hull_3(InputIterator first, InputIterator beyond,
                   Polyhedron_3& polyhedron)
{
   typedef typename std::iterator_traits<InputIterator>::value_type Point_3;
   typedef typename internal::Convex_hull_3::Default_traits_for_Chull_3<Point_3>::type Traits;
   convex_hull_3(first, beyond, polyhedron, Traits());
}

} // namespace CGAL

#endif // CGAL_CONVEX_HULL_3_H
