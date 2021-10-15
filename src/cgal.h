#pragma once

#ifdef ENABLE_CGAL

#if 0 // No extra setup needed if mimalloc is linked statically and built with MI_OVERRIDE.
      // Keeping code as an example for now, in case some platforms need it.
#ifdef USE_MIMALLOC
  #include <mimalloc.h>
  // If using CGAL_ALLOCATOR to override, then make sure to define it as the first thing
  /****** NOTE: THIS MEANS cgal.h SHOULD ALWAYS COME BEFORE OTHER CGAL INCLUDES! ******/
  #define CGAL_ALLOCATOR(t) mi_stl_allocator<t>

  // gmp requires function signature with extra oldsize parameters for some reason.
  inline void *gmp_realloc (void *ptr, size_t /*oldsize*/, size_t newsize) { return mi_realloc(ptr, newsize); }
  inline void gmp_free (void *ptr, size_t /*oldsize*/) { mi_free(ptr); }
  #include <gmp.h>
  inline void init_mimalloc() { mp_set_memory_functions(mi_malloc, gmp_realloc, gmp_free); }
#endif
#endif

#include "ext/CGAL/CGAL_workaround_Mark_bounded_volumes.h" // This file must be included prior to CGAL/Nef_polyhedron_3.h
#include <CGAL/Gmpq.h>
#include <CGAL/Extended_cartesian.h>
#include <CGAL/Nef_polyhedron_2.h>
#include <CGAL/Cartesian.h>
#include <CGAL/Polyhedron_3.h>
#include <CGAL/Nef_polyhedron_3.h>
#include <CGAL/IO/Polyhedron_iostream.h>
#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
#include <CGAL/Polygon_2.h>
#include <CGAL/Polygon_with_holes_2.h>
#include <CGAL/minkowski_sum_2.h>
#include <CGAL/minkowski_sum_3.h>
#include <CGAL/bounding_box.h>
#include <CGAL/utils.h>

#include <CGAL/assertions_behaviour.h>
#include <CGAL/exceptions.h>

typedef CGAL::Gmpq NT2;
typedef CGAL::Extended_cartesian<NT2> CGAL_Kernel2;
typedef CGAL::Nef_polyhedron_2<CGAL_Kernel2> CGAL_Nef_polyhedron2;
typedef CGAL_Kernel2::Aff_transformation_2 CGAL_Aff_transformation2;

typedef CGAL::Exact_predicates_exact_constructions_kernel CGAL_ExactKernel2;
typedef CGAL::Polygon_2<CGAL_ExactKernel2> CGAL_Poly2;
typedef CGAL::Polygon_with_holes_2<CGAL_ExactKernel2> CGAL_Poly2h;

typedef CGAL::Gmpq NT3;
typedef CGAL::Cartesian<NT3> CGAL_Kernel3;
//typedef CGAL::Exact_predicates_exact_constructions_kernel::FT NT3;
//typedef CGAL::Exact_predicates_exact_constructions_kernel CGAL_Kernel3;
typedef CGAL::Nef_polyhedron_3<CGAL_Kernel3> CGAL_Nef_polyhedron3;
typedef CGAL_Nef_polyhedron3::Aff_transformation_3 CGAL_Aff_transformation;

typedef CGAL::Polyhedron_3<CGAL_Kernel3> CGAL_Polyhedron;

typedef CGAL::Point_3<CGAL_Kernel3> CGAL_Point_3;
typedef CGAL::Triangle_3<CGAL_Kernel3> CGAL_Triangle_3;
typedef CGAL::Iso_cuboid_3<CGAL_Kernel3> CGAL_Iso_cuboid_3;
typedef std::vector<CGAL_Point_3> CGAL_Polygon_3;

// CGAL_Nef_polyhedron2 uses CGAL_Kernel2, but Iso_rectangle_2 needs to match
// CGAL_Nef_polyhedron2::Explorer::Point which is different than
// CGAL_Kernel2::Point. Hence the suffix 'e'
typedef CGAL_Nef_polyhedron2::Explorer::Point CGAL_Point_2e;
typedef CGAL::Iso_rectangle_2<CGAL::Simple_cartesian<NT2>> CGAL_Iso_rectangle_2e;

#endif /* ENABLE_CGAL */
