#pragma once

#ifdef ENABLE_CGAL

// NDEBUG must be disabled when including CGAL headers, otherwise CGAL assertions
// will not be thrown, causing OpenSCAD's CGAL error checking to fail.
// To be on the safe side, this has to be done when including any CGAL header file.
// FIXME: It might be possible to rewrite the error checking to get rid of this
// requirement. kintel 20111206.
#pragma push_macro("NDEBUG")
#undef NDEBUG
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
#pragma pop_macro("NDEBUG")

// Stringification macro helpers
#define FAST_CSG_STRING2(x) #x
#define FAST_CSG_STRING(x) FAST_CSG_STRING2(x)

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
typedef CGAL::Vector_3<CGAL_Kernel3> CGAL_Vector_3;
typedef CGAL::Triangle_3<CGAL_Kernel3> CGAL_Triangle_3;
typedef CGAL::Iso_cuboid_3<CGAL_Kernel3> CGAL_Iso_cuboid_3;
typedef std::vector<CGAL_Point_3> CGAL_Polygon_3;

// CGAL_Nef_polyhedron2 uses CGAL_Kernel2, but Iso_rectangle_2 needs to match
// CGAL_Nef_polyhedron2::Explorer::Point which is different than
// CGAL_Kernel2::Point. Hence the suffix 'e'
typedef CGAL_Nef_polyhedron2::Explorer::Point CGAL_Point_2e;
typedef CGAL::Iso_rectangle_2<CGAL::Simple_cartesian<NT2>> CGAL_Iso_rectangle_2e;

#if CGAL_VERSION_NR >= CGAL_VERSION_NUMBER(5, 1, 0)

#define FAST_CSG_AVAILABLE
// Some notes about kernels for CGALHybridPolyhedron:
// - CGAL::Epick isn't exact and gives errors with some algorithms on some models.
// - CGAL_Kernel3 (CGAL::Cartesian<CGAL::Gmpq>) uses much less memory than CGAL::Epeck
//   (see https://github.com/openscad/openscad/issues/481) so ideally we'd always
//   use it, but CGAL's corefinement functions cannot use it before 5.2.1
//   (see https://github.com/CGAL/cgal/issues/5322).
// - Conversions between CGAL::Epeck and CGAL_Kernel3 are relatively cheap
//   (see cgalutils-kernel.cc) and require -DCGAL_USE_GMPXX.
#if CGAL_VERSION_NR > CGAL_VERSION_NUMBER(5, 2, 0)
#define HYBRID_USES_EXISTING_KERNEL
typedef CGAL_Kernel3 CGAL_HybridKernel3;
#else
#pragma message( \
		"Using CGAL::Epeck as kernel for fast-csg. Compile with CGAL >5.2.0 for smaller memory footprint.")
typedef CGAL::Epeck CGAL_HybridKernel3;
#endif

#else

#pragma message("[fast-csg] No support for fast-csg with CGAL " FAST_CSG_STRING( \
		CGAL_VERSION) ". Please compile against CGAL 5.1 or later to use the feature.")

#endif

#endif /* ENABLE_CGAL */
