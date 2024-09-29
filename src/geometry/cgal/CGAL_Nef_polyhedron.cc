#include "geometry/cgal/CGAL_Nef_polyhedron.h"

#include <memory>
#include <cstddef>
#include <string>

#include "geometry/cgal/cgal.h"
#include "geometry/cgal/cgalutils.h"
#include "utils/printutils.h"
#include "utils/svg.h"

// Copy constructor only performs shallow copies, so all modifying functions
// must reset p3 with a new CGAL_Nef_polyhedron3 object, to prevent cache corruption.
// This is also partly enforced by p3 pointing to a const object.
CGAL_Nef_polyhedron::CGAL_Nef_polyhedron(const CGAL_Nef_polyhedron& src) : Geometry(src)
{
  if (src.p3) this->p3 = src.p3;
}

std::unique_ptr<Geometry> CGAL_Nef_polyhedron::copy() const
{
  return std::make_unique<CGAL_Nef_polyhedron>(*this);
}

CGAL_Nef_polyhedron CGAL_Nef_polyhedron::operator+(const CGAL_Nef_polyhedron& other) const
{
  return {std::make_shared<CGAL_Nef_polyhedron3>((*this->p3) + (*other.p3))};
}

CGAL_Nef_polyhedron& CGAL_Nef_polyhedron::operator+=(const CGAL_Nef_polyhedron& other)
{
  this->p3 = std::make_shared<CGAL_Nef_polyhedron3>((*this->p3) + (*other.p3));
  return *this;
}

CGAL_Nef_polyhedron& CGAL_Nef_polyhedron::operator*=(const CGAL_Nef_polyhedron& other)
{
  this->p3 = std::make_shared<CGAL_Nef_polyhedron3>((*this->p3) * (*other.p3));
  return *this;
}

CGAL_Nef_polyhedron& CGAL_Nef_polyhedron::operator-=(const CGAL_Nef_polyhedron& other)
{
  this->p3 = std::make_shared<CGAL_Nef_polyhedron3>((*this->p3) - (*other.p3));
  return *this;
}

// Note: this is only the fallback method in case of failure in CGALUtils::applyMinkowski (see: cgalutils-applyops.cc)
CGAL_Nef_polyhedron& CGAL_Nef_polyhedron::minkowski(const CGAL_Nef_polyhedron& other)
{
  // It is required to construct copies of our const input operands here.
  // "Postcondition: If either of the input polyhedra is non-convex, it is modified during the computation,
  //  i.e., it is decomposed into convex pieces."
  // from https://doc.cgal.org/latest/Minkowski_sum_3/group__PkgMinkowskiSum3Ref.html
  CGAL_Nef_polyhedron3 op1(*this->p3);
  CGAL_Nef_polyhedron3 op2(*other.p3);
  this->p3 = std::make_shared<CGAL_Nef_polyhedron3>(CGAL::minkowski_sum_3(op1, op2));
  return *this;
}

size_t CGAL_Nef_polyhedron::memsize() const
{
  if (this->isEmpty()) return 0;

  auto memsize = sizeof(CGAL_Nef_polyhedron);
  memsize += const_cast<CGAL_Nef_polyhedron3&>(*this->p3).bytes();
  return memsize;
}

bool CGAL_Nef_polyhedron::isEmpty() const
{
  return !this->p3 || this->p3->is_empty();
}

BoundingBox CGAL_Nef_polyhedron::getBoundingBox() const
{
  if (isEmpty()) {
    return {};
  }
  auto bb = CGALUtils::boundingBox(*this->p3).bbox();

  BoundingBox result;
  result.extend(Vector3d(bb.xmin(), bb.ymin(), bb.zmin()));
  result.extend(Vector3d(bb.xmax(), bb.ymax(), bb.zmax()));
  return result;
}

void CGAL_Nef_polyhedron::resize(const Vector3d& newsize,
                                 const Eigen::Matrix<bool, 3, 1>& autosize)
{
  // Based on resize() in Giles Bathgate's RapCAD (but not exactly)
  if (this->isEmpty()) return;

  transform(
    CGALUtils::computeResizeTransform(
      CGALUtils::boundingBox(*this->p3),
      getDimension(), newsize, autosize));
}

std::string CGAL_Nef_polyhedron::dump() const
{
  return OpenSCAD::dump_svg(*this->p3);
}

void CGAL_Nef_polyhedron::transform(const Transform3d& matrix)
{
  if (!this->isEmpty()) {
    if (matrix.matrix().determinant() == 0) {
      LOG(message_group::Warning, "Scaling a 3D object with 0 - removing object");
      this->reset();
    } else {
      auto N = std::make_shared<CGAL_Nef_polyhedron3>(*this->p3);
      CGALUtils::transform(*N, matrix);
      this->p3 = N;
    }
  }
}
