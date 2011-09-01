#ifndef CGALFWD_H_
#define CGALFWD_H_

#ifdef ENABLE_CGAL

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
	template<class T> struct Default_items;
	class SNC_indexed_items;
	template <typename Kernel_, typename Items_, typename Mark_> class Nef_polyhedron_3;
}
typedef CGAL::Cartesian<NT> CGAL_Kernel3;
typedef CGAL::Nef_polyhedron_3<CGAL_Kernel3, CGAL::SNC_indexed_items, bool> CGAL_Nef_polyhedron3;

#endif /* ENABLE_CGAL */

#endif
