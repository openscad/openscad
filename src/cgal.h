#ifndef CGAL_H_
#define CGAL_H_

#ifdef ENABLE_CGAL

#include <CGAL/Gmpq.h>
#include <CGAL/Extended_cartesian.h>
#include <CGAL/Nef_polyhedron_2.h>
#include <CGAL/Cartesian.h>
#include <CGAL/Polyhedron_3.h>
#include <CGAL/Nef_polyhedron_3.h>
#include <CGAL/IO/Polyhedron_iostream.h>

typedef CGAL::Extended_cartesian<CGAL::Gmpq> CGAL_Kernel2;
typedef CGAL::Nef_polyhedron_2<CGAL_Kernel2> CGAL_Nef_polyhedron2;
typedef CGAL_Kernel2::Aff_transformation_2 CGAL_Aff_transformation2;

typedef CGAL::Cartesian<CGAL::Gmpq> CGAL_Kernel3;
typedef CGAL::Polyhedron_3<CGAL_Kernel3> CGAL_Polyhedron;
typedef CGAL_Polyhedron::HalfedgeDS CGAL_HDS;
typedef CGAL::Polyhedron_incremental_builder_3<CGAL_HDS> CGAL_Polybuilder;
typedef CGAL::Nef_polyhedron_3<CGAL_Kernel3> CGAL_Nef_polyhedron3;
typedef CGAL_Nef_polyhedron3::Aff_transformation_3 CGAL_Aff_transformation;
typedef CGAL_Nef_polyhedron3::Vector_3 CGAL_Vector;
typedef CGAL_Nef_polyhedron3::Plane_3 CGAL_Plane;
typedef CGAL_Nef_polyhedron3::Point_3 CGAL_Point;

struct CGAL_Nef_polyhedron
{
	int dim;
	CGAL_Nef_polyhedron2 p2;
	CGAL_Nef_polyhedron3 p3;

	CGAL_Nef_polyhedron() {
		dim = 0;
	}

	CGAL_Nef_polyhedron(const CGAL_Nef_polyhedron2 &p) {
		dim = 2;
		p2 = p;
	}

	CGAL_Nef_polyhedron(const CGAL_Nef_polyhedron3 &p) {
		dim = 3;
		p3 = p;
	}

	CGAL_Nef_polyhedron& operator+=(const CGAL_Nef_polyhedron &other) {
		if (other.dim == 2) {
			this->p2 += other.p2;
			this->dim = 2;
		}
		if (other.dim == 3) {
			this->p3 += other.p3;
			this->dim = 3;
		}
		return *this;
	}

	CGAL_Nef_polyhedron& operator*=(const CGAL_Nef_polyhedron &other) {
		if (other.dim == 2) {
			this->p2 *= other.p2;
			this->dim = 2;
		}
		if (other.dim == 3) {
			this->p3 *= other.p3;
			this->dim = 3;
		}
		return *this;
	}

	CGAL_Nef_polyhedron& operator-=(const CGAL_Nef_polyhedron &other) {
		if (other.dim == 2) {
			this->p2 -= other.p2;
			this->dim = 2;
		}
		if (other.dim == 3) {
			this->p3 -= other.p3;
			this->dim = 3;
		}
		return *this;
	}

	int weight() {
		if (dim == 2)
			return p2.explorer().number_of_vertices();
		if (dim == 3)
			return p3.number_of_vertices();
		return 0;
	}
};

#endif /* ENABLE_CGAL */

#endif
