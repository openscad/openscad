// Portions of this file are Copyright 2021 Google LLC, and licensed under GPL2+. See COPYING.
#include "cgalutils.h"
#include <CGAL/Cartesian_converter.h>
#include <CGAL/gmpxx.h>

namespace CGALUtils {

template <>
double KernelConverter<CGAL::Cartesian<CGAL::Gmpq>, CGAL::Epick>::operator()(
		const CGAL::Gmpq &n) const
{
	return CGAL::to_double(n);
}

template <>
CGAL::Gmpq KernelConverter<CGAL::Epick, CGAL::Cartesian<CGAL::Gmpq>>::operator()(
		const double &n) const
{
	return n;
}

#ifndef HYBRID_USES_EXISTING_KERNEL

template <>
CGAL_HybridKernel3::FT KernelConverter<CGAL::Epick, CGAL_HybridKernel3>::operator()(
		const double &n) const
{
	return n;
}

template <>
CGAL::Epick::FT KernelConverter<CGAL_HybridKernel3, CGAL::Epick>::operator()(
		const CGAL_HybridKernel3::FT &n) const
{
	return CGAL::to_double(n);
}

template <>
CGAL_HybridKernel3::FT KernelConverter<CGAL::Cartesian<CGAL::Gmpq>, CGAL_HybridKernel3>::operator()(
		const CGAL::Gmpq &n) const
{
	return n;
}

template <>
CGAL::Gmpq KernelConverter<CGAL_HybridKernel3, CGAL::Cartesian<CGAL::Gmpq>>::operator()(
		const CGAL::Gmpq &n) const
{
	return n;
}

#endif // HYBRID_USES_EXISTING_KERNEL

} // namespace CGALUtils
