#pragma once

#ifdef ENABLE_CGAL

// STL Allocator doesn't make any significant difference on my Linux dev machine - Hans
/*
#ifdef USE_MIMALLOC
  #ifndef MI_OVERRIDE
    #include <mimalloc.h>
    // If using CGAL_ALLOCATOR to override, then make sure to define it as the first thing
    // ****** NOTE: THAT MEANS THIS FILE "cgal.h" SHOULD ALWAYS COME BEFORE OTHER CGAL INCLUDES! ******
    #define CGAL_ALLOCATOR(t) mi_stl_allocator<t>
  #endif
#endif
//*/

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
#include <CGAL/version.h>

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
// #ifdef FAST_CSG_USE_SAME_KERNEL
// typedef CGAL::Epeck CGAL_Kernel3;
// #else
typedef CGAL::Cartesian<NT3> CGAL_Kernel3;
// #endif

//typedef CGAL::Exact_predicates_exact_constructions_kernel::FT NT3;
//typedef CGAL::Exact_predicates_exact_constructions_kernel CGAL_Kernel3;
typedef CGAL::Nef_polyhedron_3<CGAL_Kernel3> CGAL_Nef_polyhedron3;
typedef CGAL_Nef_polyhedron3::Aff_transformation_3 CGAL_Aff_transformation;

typedef CGAL::Polyhedron_3<CGAL_Kernel3> CGAL_Polyhedron;

typedef CGAL::Point_3<CGAL_Kernel3> CGAL_Point_3;
typedef CGAL::Vector_3<CGAL_Kernel3> CGAL_Vector_3;
typedef CGAL::Triangle_3<CGAL_Kernel3> CGAL_Triangle_3;
typedef CGAL::Iso_cuboid_3<CGAL_Kernel3> CGAL_Iso_cuboid_3;
typedef std::vector<CGAL_Point_3> CGAL_Polygon_3;

// CGAL_Nef_polyhedron2 uses CGAL_Kernel2, but Iso_rectangle_2 needs to match
// CGAL_Nef_polyhedron2::Explorer::Point which is different than
// CGAL_Kernel2::Point. Hence the suffix 'e'
typedef CGAL_Nef_polyhedron2::Explorer::Point CGAL_Point_2e;
typedef CGAL::Iso_rectangle_2<CGAL::Simple_cartesian<NT2>> CGAL_Iso_rectangle_2e;

#if CGAL_VERSION_NR <= CGAL_VERSION_NUMBER(4, 6, 0)
#pragma error("CGAL 4.6.0 or above is required.")
#endif

// CGAL::Epeck is faster than CGAL::Cartesian<CGAL::Gmpq> (because of filtering)...
// except in some pathological cases. It can also use a lot or memory (because
// of laziness, see https://github.com/openscad/openscad/issues/481).
// For both reasons, we regularly force its numbers to exact values (either after
// each fast-csg operation, and/or inside corefinement callbacks, see
// Feature::ExperimentalFastCsgExactCallback).
//
// Conversions between CGAL::Epeck and CGAL_Kernel3 are cheap
// (see cgalutils-kernel.cc) and require -DCGAL_USE_GMPXX.
//
// Ideally we'll want to use a filtered but non-lazy kernel for all our code.
#ifdef FAST_CSG_USE_SAME_KERNEL

#if CGAL_VERSION_NR < CGAL_VERSION_NUMBER(5, 2, 0)
#pragma error("Cannot use the same kernel for corefinement before CGAL 5.2.1 "
              "(see https://github.com/CGAL/cgal/issues/5322)")
#endif // FAST_CSG_USE_SAME_KERNEL

#pragma message("[fast-csg] Using same kernel for CGAL_Kernel3 and CGAL_HybridKernel3.")

typedef CGAL_Kernel3 CGAL_HybridKernel3;

#else // not FAST_CSG_USE_SAME_KERNEL

#define FAST_CSG_DIFFERENT_KERNEL
#define FAST_CSG_KERNEL_IS_LAZY
typedef CGAL::Epeck CGAL_HybridKernel3;

#endif // FAST_CSG_USE_SAME_KERNEL

#endif /* ENABLE_CGAL */
