#include "geometry/linalg.h"
#include <cstdint>
#include <cmath>

// FIXME: We can achieve better pruning by either:
// o Recalculate the box based on the transformed object
// o Store boxes are oriented bounding boxes and implement oriented
//   bbox intersection

// FIXME: This function can be generalized, but we don't need it atm.

/*!
   Transforms the given bounding box by transforming each of its 8 vertices.
   Returns a new bounding box.
 */
BoundingBox operator*(const Transform3d& m, const BoundingBox& box)
{
  if (box.isEmpty()) return box;
  BoundingBox newbox;
  Vector3d boxvec[2] = { box.min(), box.max() };
  for (auto& k : boxvec) {
    for (auto& j : boxvec) {
      for (auto& i : boxvec) {
        newbox.extend(m * Vector3d(i[0], j[1], k[2]));
      }
    }
  }
  return newbox;
}

bool matrix_contains_infinity(const Transform3d& m)
{
  for (int i = 0; i < m.matrix().rows(); ++i) {
    for (int j = 0; j < m.matrix().cols(); ++j) {
      if ((std::isinf)(m(i, j))) return true;
    }
  }
  return false;
}

bool matrix_contains_nan(const Transform3d& m)
{
  for (int i = 0; i < m.matrix().rows(); ++i) {
    for (int j = 0; j < m.matrix().cols(); ++j) {
      if ((std::isnan)(m(i, j))) return true;
    }
  }
  return false;
}

/* Hash a floating point number, copied almost line by line from
   Python's pyhash.c, originally by the Python team, Mark Dickinson, et al.
   The copyright License for this code can be found in under
   ../doc/Python-LICENSE.TXT. Changes from the original code include
   de-scattering typedefs, and removing the -1 special return case.

   This is designed so srand() will work better with floating point
   numbers. An oversimplified explanation is that the code calculates the
   Remainder of the input divided by 2^31, in a very portable way. See also:

   http://bob.ippoli.to/archives/2010/03/23/py3k-unified-numeric-hash/
   https://github.com/python/cpython/blob/master/Python/pyhash.c
   https://github.com/python/cpython/blob/master/Include/pyhash.h
   http://stackoverflow.com/questions/4238122/hash-function-for-floats
   http://betterexplained.com/articles/fun-with-modular-arithmetic/
 */
using Py_hash_t = int32_t;
using Py_uhash_t = uint32_t;
using Float_t = double;
Py_hash_t hash_floating_point(Float_t v)
{
  static constexpr int PyHASH_BITS = 31;
  //if (sizeof(Py_uhash_t)==8) PyHASH_BITS=61;

  static constexpr Py_uhash_t PyHASH_MODULUS = (((Py_uhash_t)1 << PyHASH_BITS) - 1);
  static constexpr Py_uhash_t PyHASH_INF = 314159;
  static constexpr Py_uhash_t PyHASH_NAN = 0;

  int e, sign;
  Float_t m;
  Py_uhash_t x, y;

  if (!std::isfinite(v)) {
    if (std::isinf(v)) return v > 0 ? PyHASH_INF : -PyHASH_INF;
    else return PyHASH_NAN;
  }

  m = frexp(v, &e);

  sign = 1;
  if (m < 0) {
    sign = -1;
    m = -m;
  }

  /* process 28 bits at a time;  this should work well both for binary
     and hexadecimal floating point. */
  x = 0;
  while (m) {
    x = ((x << 28) & PyHASH_MODULUS) | x >> (PyHASH_BITS - 28);
    m *= 268435456.0; // 2**28
    e -= 28;
    y = (Py_uhash_t)m; /* pull out integer part */
    m -= y;
    x += y;
    if (x >= PyHASH_MODULUS) x -= PyHASH_MODULUS;
  }

  /* adjust for the exponent;  first reduce it modulo PyHASH_BITS */
  e = e >= 0 ? e % PyHASH_BITS : PyHASH_BITS - 1 - ((-1 - e) % PyHASH_BITS);
  x = ((x << e) & PyHASH_MODULUS) | x >> (PyHASH_BITS - e);

  x = x * sign;
  return (Py_hash_t)x;
}
