#include "CGAL_Nef_polyhedron.h"
#include "cgal.h"
#include "cgalutils.h"
#include "printutils.h"
#include "polyset.h"
#include "svg.h"

CGAL_Nef_polyhedron::CGAL_Nef_polyhedron(CGAL_Nef_polyhedron3 *p)
{
	if (p) p3.reset(p);
}

// Copy constructor
CGAL_Nef_polyhedron::CGAL_Nef_polyhedron(const CGAL_Nef_polyhedron &src)
{
	if (src.p3) this->p3.reset(new CGAL_Nef_polyhedron3(*src.p3));
}

CGAL_Nef_polyhedron &CGAL_Nef_polyhedron::operator+=(const CGAL_Nef_polyhedron &other)
{
	(*this->p3) += (*other.p3);
	return *this;
}

CGAL_Nef_polyhedron &CGAL_Nef_polyhedron::operator*=(const CGAL_Nef_polyhedron &other)
{
	(*this->p3) *= (*other.p3);
	return *this;
}

CGAL_Nef_polyhedron &CGAL_Nef_polyhedron::operator-=(const CGAL_Nef_polyhedron &other)
{
	(*this->p3) -= (*other.p3);
	return *this;
}

CGAL_Nef_polyhedron &CGAL_Nef_polyhedron::minkowski(const CGAL_Nef_polyhedron &other)
{
	(*this->p3) = CGAL::minkowski_sum_3(*this->p3, *other.p3);
	return *this;
}

size_t CGAL_Nef_polyhedron::memsize() const
{
	if (this->isEmpty()) return 0;

	auto memsize = sizeof(CGAL_Nef_polyhedron);
	memsize += this->p3->bytes();
	return memsize;
}

bool CGAL_Nef_polyhedron::isEmpty() const
{
	return !this->p3 || this->p3->is_empty();
}

/*!
   Creates a new PolySet and initializes it with the data from this polyhedron

   Note: Can return nullptr if an error occurred
 */
// FIXME: Deprecated by CGALUtils::createPolySetFromNefPolyhedron3
#if 0
PolySet *CGAL_Nef_polyhedron::convertToPolyset() const
{
	if (this->isEmpty()) return new PolySet(3);
	PolySet *ps = nullptr;
	CGAL::Failure_behaviour old_behaviour = CGAL::set_error_behaviour(CGAL::THROW_EXCEPTION);
	ps = new PolySet(3);
	ps->setConvexity(this->convexity);
	bool err = true;
	std::string errmsg("");
	CGAL_Polyhedron P;
	try {
		// Cast away constness:
		// convert_to_Polyhedron() wasn't const in earlier versions of CGAL.
		CGAL_Nef_polyhedron3 *nonconst_nef3 = const_cast<CGAL_Nef_polyhedron3 *>(this->p3.get());
		err = nefworkaround::convert_to_Polyhedron<CGAL_Kernel3>(*(nonconst_nef3), P);
		//this->p3->convert_to_Polyhedron(P);
	}
	catch (const CGAL::Failure_exception &e) {
		err = true;
		errmsg = std::string(e.what());
	}
	if (!err) err = CGALUtils::createPolySetFromPolyhedron(P, *ps);
	if (err) {
		PRINT("ERROR: CGAL NefPolyhedron->Polyhedron conversion failed.");
		if (errmsg != "") PRINTB("ERROR: %s", errmsg);
		delete ps; ps = nullptr;
	}
	CGAL::set_error_behaviour(old_behaviour);
	return ps;
}
#endif // if 0

void CGAL_Nef_polyhedron::resize(const Vector3d &newsize,
																 const Eigen::Matrix<bool, 3, 1> &autosize)
{
	// Based on resize() in Giles Bathgate's RapCAD (but not exactly)
	if (this->isEmpty()) return;

	auto bb = CGALUtils::boundingBox(*this->p3);

	std::vector<NT3> scale, bbox_size;
	for (unsigned int i = 0; i < 3; i++) {
		scale.push_back(NT3(1));
		bbox_size.push_back(bb.max_coord(i) - bb.min_coord(i));
	}
	int newsizemax_index = 0;
	for (unsigned int i = 0; i < this->getDimension(); i++) {
		if (newsize[i]) {
			if (bbox_size[i] == NT3(0)) {
				PRINT("WARNING: Resize in direction normal to flat object is not implemented");
				return;
			}
			else {
				scale[i] = NT3(newsize[i]) / bbox_size[i];
			}
			if (newsize[i] > newsize[newsizemax_index]) newsizemax_index = i;
		}
	}

	auto autoscale = NT3(1);
	if (newsize[newsizemax_index] != 0) {
		autoscale = NT3(newsize[newsizemax_index]) / bbox_size[newsizemax_index];
	}
	for (unsigned int i = 0; i < this->getDimension(); i++) {
		if (autosize[i] && newsize[i] == 0) scale[i] = autoscale;
	}

	Eigen::Matrix4d t;
	t << CGAL::to_double(scale[0]),           0,        0,        0,
		0,        CGAL::to_double(scale[1]),           0,        0,
		0,        0,        CGAL::to_double(scale[2]),           0,
		0,        0,        0,                                   1;

	this->transform(Transform3d(t));
}

std::string CGAL_Nef_polyhedron::dump() const
{
	return OpenSCAD::dump_svg(*this->p3);
}


void CGAL_Nef_polyhedron::transform(const Transform3d &matrix)
{
	if (!this->isEmpty()) {
		if (matrix.matrix().determinant() == 0) {
			PRINT("WARNING: Scaling a 3D object with 0 - removing object");
			this->reset();
		}
		else {
			CGAL_Aff_transformation t(
				matrix(0, 0), matrix(0, 1), matrix(0, 2), matrix(0, 3),
				matrix(1, 0), matrix(1, 1), matrix(1, 2), matrix(1, 3),
				matrix(2, 0), matrix(2, 1), matrix(2, 2), matrix(2, 3), matrix(3, 3));
			this->p3->transform(t);
		}
	}
}
