// Portions of this file are Copyright 2021 Google LLC, and licensed under GPL2+. See COPYING.
#include "cgalutils.h"
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

template <>
CGAL::Lazy_exact_nt<mpq_class> KernelConverter<CGAL::Cartesian<CGAL::Gmpq>,
																							 CGAL::Epeck>::operator()(const CGAL::Gmpq &n) const
{
	return mpq_class(mpz_class(n.numerator().mpz()), mpz_class(n.denominator().mpz()));
}

template <>
CGAL::Gmpq KernelConverter<CGAL::Epeck, CGAL::Cartesian<CGAL::Gmpq>>::operator()(
		const CGAL::Lazy_exact_nt<mpq_class> &n) const
{
	auto &e = n.exact();
	return CGAL::Gmpq(CGAL::Gmpz(e.get_num().get_mpz_t()), CGAL::Gmpz(e.get_den().get_mpz_t()));
}

} // namespace CGALUtils
