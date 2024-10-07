// Portions of this file are Copyright 2021 Google LLC, and licensed under GPL2+. See COPYING.
#include "geometry/cgal/cgalutils.h"
#include <CGAL/Cartesian_converter.h>
#include <CGAL/gmpxx.h>

namespace CGALUtils {

template <>
double KernelConverter<CGAL::Cartesian<CGAL::Gmpq>, CGAL::Epick>::operator()(
  const CGAL::Gmpq& n) const
{
  return CGAL::to_double(n);
}

template <>
CGAL::Gmpq KernelConverter<CGAL::Epick, CGAL::Cartesian<CGAL::Gmpq>>::operator()(
  const double& n) const
{
  return n;
}

template <>
CGAL_HybridKernel3::FT KernelConverter<CGAL::Epick, CGAL_HybridKernel3>::operator()(
  const double& n) const
{
  return n;
}

template <>
double KernelConverter<CGAL::Epick, CGAL_DoubleKernel>::operator()(
  const double& n) const
{
  return n;
}

template <>
CGAL::Epick::FT KernelConverter<CGAL_HybridKernel3, CGAL::Epick>::operator()(
  const CGAL_HybridKernel3::FT& n) const
{
  return CGAL::to_double(n);
}

template <>
CGAL_HybridKernel3::FT KernelConverter<CGAL_Kernel3, CGAL_HybridKernel3>::operator()(
  const CGAL::Gmpq& n) const
{
  return mpq_class(mpz_class(n.numerator().mpz()), mpz_class(n.denominator().mpz()));
}

template <>
CGAL::Gmpq KernelConverter<CGAL_HybridKernel3, CGAL_Kernel3>::operator()(
  const CGAL_HybridKernel3::FT& n) const
{
  auto& e = n.exact();
  return {CGAL::Gmpz(e.get_num().get_mpz_t()), CGAL::Gmpz(e.get_den().get_mpz_t())};
}

} // namespace CGALUtils
