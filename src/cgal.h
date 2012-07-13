#ifndef CGAL_H_
#define CGAL_H_

#ifdef ENABLE_CGAL

#ifdef _MSC_VER
// see http://en.wikipedia.org/wiki/Stdint.h
// and http://www.mpfr.org/mpfr-2.4.2/#stdint
#include <boost/cstdint.hpp>
using boost::intmax_t;
using boost::uintmax_t;
#endif

// NDEBUG must be disabled when including CGAL headers, otherwise CGAL assertions
// will not be thrown, causing OpenSCAD's CGAL error checking to fail.
// To be on the safe side, this has to be done when including any CGAL header file.
// FIXME: It might be possible to rewrite the error checking to get rid of this
// requirement. kintel 20111206.
#ifdef NDEBUG
#define PREV_NDEBUG NDEBUG
#undef NDEBUG
#endif

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

#include <CGAL/assertions_behaviour.h>
#include <CGAL/exceptions.h>

typedef CGAL::Gmpq NT;
typedef CGAL::Extended_cartesian<NT> CGAL_Kernel2;
typedef CGAL::Nef_polyhedron_2<CGAL_Kernel2> CGAL_Nef_polyhedron2;
typedef CGAL_Kernel2::Aff_transformation_2 CGAL_Aff_transformation2;

typedef CGAL::Exact_predicates_exact_constructions_kernel CGAL_ExactKernel2;
typedef CGAL::Polygon_2<CGAL_ExactKernel2> CGAL_Poly2;
typedef CGAL::Polygon_with_holes_2<CGAL_ExactKernel2> CGAL_Poly2h;

typedef CGAL::Cartesian<NT> CGAL_Kernel3;
typedef CGAL::Nef_polyhedron_3<CGAL_Kernel3> CGAL_Nef_polyhedron3;
typedef CGAL_Nef_polyhedron3::Aff_transformation_3 CGAL_Aff_transformation;

typedef CGAL::Polyhedron_3<CGAL_Kernel3> CGAL_Polyhedron;
typedef CGAL_Polyhedron::HalfedgeDS CGAL_HDS;
typedef CGAL::Polyhedron_incremental_builder_3<CGAL_HDS> CGAL_Polybuilder;

#ifdef PREV_NDEBUG
#define NDEBUG PREV_NDEBUG
#endif

#endif /* ENABLE_CGAL */

#endif
