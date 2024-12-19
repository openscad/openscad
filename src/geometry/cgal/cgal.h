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

#include "CGAL/CGAL_workaround_Mark_bounded_volumes.h" // This file must be included prior to CGAL/Nef_polyhedron_3.h
#include <vector>
#include <CGAL/Gmpq.h>
#include <CGAL/Extended_cartesian.h>
#include <CGAL/Nef_polyhedron_2.h>
#include <CGAL/Cartesian.h>
#include <CGAL/Polyhedron_3.h>
#include <CGAL/Nef_polyhedron_3.h>
#include <CGAL/Surface_mesh.h>
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

using NT2 = CGAL::Gmpq;
using CGAL_Kernel2 = CGAL::Extended_cartesian<NT2>;
using CGAL_Nef_polyhedron2 = CGAL::Nef_polyhedron_2<CGAL_Kernel2>;
using CGAL_Aff_transformation2 = CGAL_Kernel2::Aff_transformation_2;

using CGAL_ExactKernel2 = CGAL::Exact_predicates_exact_constructions_kernel;
using CGAL_Poly2 = CGAL::Polygon_2<CGAL_ExactKernel2>;
using CGAL_Poly2h = CGAL::Polygon_with_holes_2<CGAL_ExactKernel2>;

using NT3 = CGAL::Gmpq;
using CGAL_Kernel3 = CGAL::Cartesian<NT3>;
//using NT3 = CGAL::Exact_predicates_exact_constructions_kernel::FT;
//using CGAL_Kernel3 = CGAL::Exact_predicates_exact_constructions_kernel;
using CGAL_Nef_polyhedron3 = CGAL::Nef_polyhedron_3<CGAL_Kernel3>;
using CGAL_Aff_transformation = CGAL_Nef_polyhedron3::Aff_transformation_3;

using CGAL_Polyhedron = CGAL::Polyhedron_3<CGAL_Kernel3>;

using CGAL_Point_3 = CGAL::Point_3<CGAL_Kernel3>;
using CGAL_Triangle_3 = CGAL::Triangle_3<CGAL_Kernel3>;
using CGAL_Iso_cuboid_3 = CGAL::Iso_cuboid_3<CGAL_Kernel3>;
using CGAL_Polygon_3 = std::vector<CGAL_Point_3>;

// CGAL_Nef_polyhedron2 uses CGAL_Kernel2, but Iso_rectangle_2 needs to match
// CGAL_Nef_polyhedron2::Explorer::Point which is different than
// CGAL_Kernel2::Point. Hence the suffix 'e'
using CGAL_Point_2e = CGAL_Nef_polyhedron2::Explorer::Point;
using CGAL_Iso_rectangle_2e = CGAL::Iso_rectangle_2<CGAL::Simple_cartesian<NT2>>;

#define FAST_CSG_KERNEL_IS_LAZY 1
using CGAL_HybridKernel3 = CGAL::Epeck;

using CGAL_HybridPoint = CGAL::Point_3<CGAL_HybridKernel3>;
using CGAL_HybridNef = CGAL::Nef_polyhedron_3<CGAL_HybridKernel3>;
using CGAL_HybridMesh = CGAL::Surface_mesh<CGAL_HybridPoint>;

using CGAL_DoubleKernel = CGAL::Cartesian<double>;
using CGAL_DoublePoint3 = CGAL_DoubleKernel::Point_3;
using CGAL_DoubleMesh = CGAL::Surface_mesh<CGAL_DoublePoint3>;

#endif /* ENABLE_CGAL */
