// version_check.h by don bright 2012. Copyright assigned to Marius Kintel and
// Clifford Wolf 2012. Released under the GPL 2, or later, as described in
// the file named 'COPYING' in OpenSCAD's project root.

/* This file will check versions of libraries at compile time. If they
   are too old, the user will be warned. If the user wishes to force
   compilation, they can run

   qmake CONFIG+=skip-version-check

   Otherwise they will be guided to README.md and an -build-dependencies script.

   The extensive #else #endif is to ensure only a single error is printed at
   a time, to avoid confusion.
 */

#pragma once

#ifndef OPENSCAD_SKIP_VERSION_CHECK

#include <stddef.h> // Needed by gmp.h under OS X 10.10
#include <gmp.h>
// set minimum numbers here.
#define GMPMAJOR 5
#define GMPMINOR 0
#define GMPPATCH 0
#define SYS_GMP_VER  (__GNU_MP_VERSION * 10000 + __GNU_MP_VERSION_MINOR * 100 + __GNU_MP_VERSION_PATCHLEVEL * 1)
#if SYS_GMP_VER < GMPMAJOR * 10000 + GMPMINOR * 100 + GMPPATCH * 1
#error GNU GMP library missing or version too old. See README.md. To force compile, run qmake CONFIG+=skip-version-check
#else


#include <mpfr.h>
#if MPFR_VERSION < MPFR_VERSION_NUM(3, 0, 0)
#error GNU MPFR library missing or version too old. See README.md. To force compile, run qmake CONFIG+=skip-version-check
#else


#include <Eigen/Core>
#if not EIGEN_VERSION_AT_LEAST(3, 0, 0)
#error eigen library missing or version too old. See README.md. To force compile, run qmake CONFIG+=skip-version-check
#else


#include <boost/version.hpp>
// boost 1.3.5 = 103500
#if BOOST_VERSION < 103500
#error boost library missing or version too old. See README.md. To force compile, run qmake CONFIG+=skip-version-check
#else


#ifdef ENABLE_CGAL
#include <CGAL/version.h>

#if CGAL_VERSION_NR < 1030601000
#error CGAL library missing or version too old. See README.md. To force compile, run qmake CONFIG+=skip-version-check
#else

#if CGAL_VERSION_NR < 1040021000
#warning "======================="
#warning "."
#warning "."
#warning "."
#warning "."
#warning CGAL library version is old, risking buggy behavior. Please see README.md. Continuing anyway.
#warning "."
#warning "."
#warning "."
#warning "."
#warning "======================="
#ifdef __clang__
#error For Clang to work, CGAL must be >= 4.0.2
#endif
#endif // CGAL_VERSION_NR < 10400010000
#endif //ENABLE_CGAL

#ifdef ENABLE_OPENCSG
#include <GL/glew.h>
// kludge - GLEW doesnt have compiler-accessible version numbering
#ifndef GLEW_ARB_occlusion_query2
#error GLEW library missing or version too old. See README.md. To force compile, run qmake CONFIG+=skip-version-check
#else


#include <opencsg.h>
// 1.3.2 -> 0x0132
#if OPENCSG_VERSION < 0x0132
#error OPENCSG library missing or version too old. See README.md. To force compile, run qmake CONFIG+=skip-version-check
#else
#endif // ENABLE_OPENCSG

#ifndef OPENSCAD_NOGUI
#include <QtCore/qglobal.h>
#if QT_VERSION < 0x040400
#error QT library missing or version too old. See README.md. To force compile, run qmake CONFIG+=skip-version-check
#endif // QT
#endif

#ifdef ENABLE_OPENCSG
#endif // OpenCSG
#endif // GLEW
#endif // ENABLE_OPENCSG

#ifdef ENABLE_CGAL
#endif // CGAL error
#endif // ENABLE_CGAL

#endif // Boost
#endif // Eigen
#endif // MPFR
#endif // GMP

// see github issue #552
#define GCC_VERSION (__GNUC__ * 10000 \
										 + __GNUC_MINOR__ * 100 \
										 + __GNUC_PATCHLEVEL__)
#if GCC_VERSION == 40802
#warning "gcc 4.8.2 contains a bug causing a crash in CGAL."
#endif

#endif // OPENSCAD_SKIP_VERSION_CHECK

