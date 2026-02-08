// Portions of this file are Copyright 2021 Google LLC, and licensed under GPL2+. See COPYING.
#include "geometry/cgal/cgal.h"
#include "geometry/cgal/cgalutils.h"
#include <CGAL/Cartesian_converter.h>
#include <CGAL/gmpxx.h>

namespace CGALUtils {

template <>
double KernelConverter<CGAL_Kernel3, CGAL::Epick>::operator()(const CGAL_Kernel3::FT& n) const
{
  return CGAL::to_double(n);
}

template <>
CGAL_Kernel3::FT KernelConverter<CGAL::Epick, CGAL_Kernel3>::operator()(const double& n) const
{
  return n;
}

template <>
double KernelConverter<CGAL::Epick, CGAL_DoubleKernel>::operator()(const double& n) const
{
  return n;
}

}  // namespace CGALUtils
