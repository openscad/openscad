// Portions of this file are Copyright 2021 Google LLC, and licensed under GPL2+. See COPYING.
#include "geometry/cgal/cgalutils.h"
#include <CGAL/Cartesian_converter.h>
#include <CGAL/gmpxx.h>

namespace CGALUtils {

template <>
double KernelConverter<CGAL::Cartesian<CGAL::Gmpq>, CGAL::Epick>::operator()(const CGAL::Gmpq& n) const
{
  return CGAL::to_double(n);
}

template <>
CGAL::Gmpq KernelConverter<CGAL::Epick, CGAL::Cartesian<CGAL::Gmpq>>::operator()(const double& n) const
{
  return n;
}

template <>
double KernelConverter<CGAL::Epick, CGAL_DoubleKernel>::operator()(const double& n) const
{
  return n;
}

}  // namespace CGALUtils
