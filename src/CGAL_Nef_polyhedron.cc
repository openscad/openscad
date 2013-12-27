#include "CGAL_Nef_polyhedron.h"
#include "cgal.h"
#include "cgalutils.h"
#include "printutils.h"
#include "polyset.h"

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
	if (this->dim == 3) (*this->p3) += (*other.p3);
	return *this;
}

CGAL_Nef_polyhedron& CGAL_Nef_polyhedron::operator*=(const CGAL_Nef_polyhedron &other)
{
	if (this->dim == 3) (*this->p3) *= (*other.p3);
	return *this;
}

CGAL_Nef_polyhedron& CGAL_Nef_polyhedron::operator-=(const CGAL_Nef_polyhedron &other)
{
	if (this->dim == 3) (*this->p3) -= (*other.p3);
	return *this;
}

extern CGAL_Nef_polyhedron2 minkowski2(const CGAL_Nef_polyhedron2 &a, const CGAL_Nef_polyhedron2 &b);

CGAL_Nef_polyhedron &CGAL_Nef_polyhedron::minkowski(const CGAL_Nef_polyhedron &other)
{
	if (this->dim == 3) (*this->p3) = CGAL::minkowski_sum_3(*this->p3, *other.p3);
	return *this;
}

size_t CGAL_Nef_polyhedron::memsize() const
{
	if (this->isNull()) return 0;

	size_t memsize = sizeof(CGAL_Nef_polyhedron);
	if (this->dim == 3) memsize += this->p3->bytes();
	return memsize;
}

/*!
	Creates a new PolySet and initializes it with the data from this polyhedron

	Note: Can return NULL if an error occurred
*/
PolySet *CGAL_Nef_polyhedron::convertToPolyset() const
{
	if (this->isNull()) return new PolySet();
	PolySet *ps = NULL;
	if (this->dim == 3) {
		CGAL::Failure_behaviour old_behaviour = CGAL::set_error_behaviour(CGAL::THROW_EXCEPTION);
		ps = new PolySet();
		ps->setConvexity(this->convexity);
		bool err = true;
		std::string errmsg("");
		CGAL_Polyhedron P;
		try {
      // Cast away constness: 
			// convert_to_Polyhedron() wasn't const in earlier versions of CGAL.
			CGAL_Nef_polyhedron3 *nonconst_nef3 = const_cast<CGAL_Nef_polyhedron3*>(this->p3.get());
			err = nefworkaround::convert_to_Polyhedron<CGAL_Kernel3>( *(nonconst_nef3), P );
			//this->p3->convert_to_Polyhedron(P);
		}
		catch (const CGAL::Failure_exception &e) {
			err = true;
			errmsg = std::string(e.what());
		}
		if (!err) err = createPolySetFromPolyhedron(P, *ps);
		if (err) {
			PRINT("ERROR: CGAL NefPolyhedron->Polyhedron conversion failed.");
			if (errmsg!="") PRINTB("ERROR: %s",errmsg);
			delete ps; ps = NULL;
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
	if (copy.p3) copy.p3.reset(new CGAL_Nef_polyhedron3(*copy.p3));
	return copy;
}
