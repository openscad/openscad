#pragma once

#ifndef CGAL_FORWARD
#include "cgal.h"
#else
#ifdef ENABLE_CGAL

#include <memory>

namespace CGAL {
class Gmpq;
template <class T> class Extended_cartesian;
class HDS_items;
template <class A, typename Items_, typename Mark_> class Nef_polyhedron_2;
}
typedef CGAL::Gmpq NT;
typedef CGAL::Extended_cartesian<NT> CGAL_Kernel2;
typedef CGAL::Nef_polyhedron_2<CGAL_Kernel2, CGAL::HDS_items, bool> CGAL_Nef_polyhedron2;

namespace CGAL {
template <class T> class Cartesian;
template <class T> struct Default_items;
class SNC_indexed_items;
template <typename Kernel_, typename Items_, typename Mark_> class Nef_polyhedron_3;
}
typedef CGAL::Cartesian<NT> CGAL_Kernel3;
typedef CGAL::Nef_polyhedron_3<CGAL_Kernel3, CGAL::SNC_indexed_items, bool> CGAL_Nef_polyhedron3;

namespace CGAL {
#ifndef CGAL_ALLOCATOR
#  define CGAL_ALLOCATOR(T) std::allocator<T>
#endif
class HalfedgeDS_items_2;
template <class Traits_, class HalfedgeDSItems, class Alloc> class HalfedgeDS_default;
class Polyhedron_items_3;
template <class PolyhedronTraits_3, class PolyhedronItems_3, class T_HDS, class Alloc> class Polyhedron_3;
}
typedef CGAL::Polyhedron_3<CGAL_Kernel3, CGAL::Polyhedron_items_3, CGAL::HalfedgeDS_default<CGAL_Kernel3, CGAL::HalfedgeDS_items_2, CGAL_ALLOCATOR(int)>, CGAL_ALLOCATOR(int)> CGAL_Polyhedron;

#endif /* ENABLE_CGAL */

#endif // ifndef CGAL_FORWARD
