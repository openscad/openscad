#include "CGAL_Nef_polyhedron.h"
#include "cgal.h"
#include "cgalutils.h"
#include "printutils.h"
#include "polyset.h"
#include "dxfdata.h"
#include "dxftess.h"

CGAL_Nef_polyhedron::CGAL_Nef_polyhedron(CGAL_Nef_polyhedron2 *p)
{
	if (p) {
		dim = 2;
		p2.reset(p);
	}
	else {
		dim = 0;
	}
}

CGAL_Nef_polyhedron::CGAL_Nef_polyhedron(CGAL_Nef_polyhedron3 *p)
{
	if (p) {
		dim = 3;
		p3.reset(p);
	}
	else {
		dim = 0;
	}
}


CGAL_Nef_polyhedron& CGAL_Nef_polyhedron::operator+=(const CGAL_Nef_polyhedron &other)
{
	if (this->dim == 2) (*this->p2) += (*other.p2);
	else if (this->dim == 3) (*this->p3) += (*other.p3);
	return *this;
}

CGAL_Nef_polyhedron& CGAL_Nef_polyhedron::operator*=(const CGAL_Nef_polyhedron &other)
{
	if (this->dim == 2) (*this->p2) *= (*other.p2);
	else if (this->dim == 3) (*this->p3) *= (*other.p3);
	return *this;
}

CGAL_Nef_polyhedron& CGAL_Nef_polyhedron::operator-=(const CGAL_Nef_polyhedron &other)
{
	if (this->dim == 2) (*this->p2) -= (*other.p2);
	else if (this->dim == 3) (*this->p3) -= (*other.p3);
	return *this;
}

extern CGAL_Nef_polyhedron2 minkowski2(const CGAL_Nef_polyhedron2 &a, const CGAL_Nef_polyhedron2 &b);

CGAL_Nef_polyhedron &CGAL_Nef_polyhedron::minkowski(const CGAL_Nef_polyhedron &other)
{
	if (this->dim == 2) (*this->p2) = minkowski2(*this->p2, *other.p2);
	else if (this->dim == 3) (*this->p3) = CGAL::minkowski_sum_3(*this->p3, *other.p3);
	return *this;
}

int CGAL_Nef_polyhedron::weight() const
{
	if (this->empty()) return 0;

	size_t memsize = sizeof(CGAL_Nef_polyhedron);
	if (this->dim == 2) {
		memsize += sizeof(CGAL_Nef_polyhedron2) +
			this->p2->explorer().number_of_vertices() * sizeof(CGAL_Nef_polyhedron2::Explorer::Vertex) +
			this->p2->explorer().number_of_halfedges() * sizeof(CGAL_Nef_polyhedron2::Explorer::Halfedge) +
			this->p2->explorer().number_of_edges() * sizeof(CGAL_Nef_polyhedron2::Explorer::Face);
	}
	if (this->dim == 3) memsize += this->p3->bytes();
	return memsize;
}

/*!
	Creates a new PolySet and initializes it with the data from this polyhedron

	This method is not const since convert_to_Polyhedron() wasn't const
  in earlier versions of CGAL.

	Note: Can return NULL if an error occurred
*/
PolySet *CGAL_Nef_polyhedron::convertToPolyset()
{
	assert(!this->empty());
	PolySet *ps = NULL;
	if (this->dim == 2) {
		ps = new PolySet();
		DxfData *dd = this->convertToDxfData();
		ps->is2d = true;
		dxf_tesselate(ps, *dd, 0, true, false, 0);
		dxf_border_to_ps(ps, *dd);
		delete dd;
	}
	else if (this->dim == 3) {
		CGAL::Failure_behaviour old_behaviour = CGAL::set_error_behaviour(CGAL::THROW_EXCEPTION);
		try {
			CGAL_Polyhedron P;
			this->p3->convert_to_Polyhedron(P);
			ps = createPolySetFromPolyhedron(P);
		}
		catch (const CGAL::Precondition_exception &e) {
			PRINTB("CGAL error in CGAL_Nef_polyhedron::convertToPolyset(): %s", e.what());
		}
		CGAL::set_error_behaviour(old_behaviour);
	}
	return ps;
}

/*!
	Deep copy
*/
CGAL_Nef_polyhedron CGAL_Nef_polyhedron::copy() const
{
	CGAL_Nef_polyhedron copy = *this;
	if (copy.p2) copy.p2.reset(new CGAL_Nef_polyhedron2(*copy.p2));
	else if (copy.p3) copy.p3.reset(new CGAL_Nef_polyhedron3(*copy.p3));
	return copy;
}
