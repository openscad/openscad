"""Vector and matrix helper types for the PythonSCAD API.

These classes give scripts a convenient, well-typed way to build the
coordinate/matrix arguments that the PythonSCAD API accepts, while
transparently bridging between plain Python lists and NumPy arrays:

* When NumPy is installed the classes are **NumPy-backed** -- every
  instance is a genuine :class:`numpy.ndarray` subclass, so array maths
  (``v1 + v2``, ``m @ v``) works.
* When NumPy is **not** installed the classes fall back to :class:`list`
  subclasses. They still accept lists (or anything list-like) and are
  themselves plain lists, so the same script keeps working without NumPy --
  values are seamlessly converted to/from lists.

In both cases every constructor accepts a Python list, tuple, another
vector/matrix or (when available) a NumPy array, and validates the
dimensionality.

This module is a submodule of the ``pythonscad`` package because these
helpers are PythonSCAD-specific rather than part of OpenSCAD's Python API.
It imports only :mod:`numpy` (optionally), so it can be loaded directly by
file path for unit testing without importing the package (and hence without
the compiled ``_openscad`` extension).

Public names: :data:`Vector1`, :data:`Vector2`, :data:`Vector3`,
:class:`Matrix4x4` and the :data:`HAS_NUMPY` flag.
"""

from __future__ import annotations

try:  # NumPy is an optional runtime dependency.
    import numpy as _np

    HAS_NUMPY = True
except ImportError:  # pragma: no cover - exercised via monkeypatching in tests
    _np = None
    HAS_NUMPY = False


__all__ = ["Vector1", "Vector2", "Vector3", "Matrix4x4", "HAS_NUMPY"]


def _as_plain_list(value):
    """Best-effort conversion of a comparison operand to a plain Python list.

    Raises (caught by callers) for non-sequence operands so comparisons with
    scalars fall back to ``NotImplemented``.
    """
    if hasattr(value, "tolist"):
        return value.tolist()
    return list(value)


if HAS_NUMPY:

    class _ScadArray(_np.ndarray):
        """Base ndarray subclass for the NumPy-backed vector/matrix types.

        Presents like the plain-list types so scripts (and printed output)
        behave the same whether or not NumPy is installed:

        * ``repr`` / ``str`` render as a plain Python list of lists.
        * ``__iter__`` yields plain Python scalars/lists (via ``tolist()``)
          rather than ``numpy`` scalars, so ``list(v)`` and ``for x in v``
          produce ``[10.0, 20.0, 30.0]`` and ``10.0`` -- not ``np.float64(...)``.
        * ``__eq__`` returns a plain ``bool`` (list comparison) rather than
          NumPy's element-wise boolean array, so ``if v == [1, 2, 3]:`` works
          instead of raising "truth value of an array is ambiguous".
        """

        def __repr__(self):
            return repr(self.tolist())

        def __str__(self):
            return str(self.tolist())

        def __iter__(self):
            return iter(self.tolist())

        def __eq__(self, other):
            try:
                return self.tolist() == _as_plain_list(other)
            except Exception:
                return NotImplemented

        def __ne__(self, other):
            result = self.__eq__(other)
            if result is NotImplemented:
                return result
            return not result

        # ndarray sets __hash__ = None (unhashable); overriding __eq__ would
        # otherwise re-expose object hashing, so keep these values unhashable.
        __hash__ = None

    class _NumpyVector(_ScadArray):
        """Fixed-length 1-D vector backed by :class:`numpy.ndarray`."""

        _length: int = 0

        def __new__(cls, iterable=None):
            if iterable is None:
                data = _np.zeros(cls._length, dtype=float)
            else:
                data = _np.asarray(iterable, dtype=float).ravel()
            if cls._length and data.size != cls._length:
                raise ValueError(
                    f"{cls.__name__} must have exactly {cls._length} elements"
                )
            # copy() so the instance owns its data and later in-place edits
            # never mutate a caller-supplied array.
            return data.copy().view(cls)

        @classmethod
        def from_array(cls, array):
            """Build the vector from a NumPy array (or any list-like)."""
            return cls(array)

    class Vector1(_NumpyVector):
        """1D vector ``[x]`` (NumPy-backed)."""

        _length = 1

    class Vector2(_NumpyVector):
        """2D vector ``[x, y]`` (NumPy-backed)."""

        _length = 2

    class Vector3(_NumpyVector):
        """3D vector ``[x, y, z]`` (NumPy-backed)."""

        _length = 3

    class Matrix4x4(_ScadArray):
        """4x4 transformation matrix backed by :class:`numpy.ndarray`."""

        def __new__(cls, iterable=None):
            if iterable is None:
                data = _np.identity(4, dtype=float)
            else:
                data = _np.asarray(iterable, dtype=float)
                if data.shape != (4, 4):
                    raise ValueError("Matrix4x4 must be a 4x4 matrix")
            return data.copy().view(cls)

        @classmethod
        def from_array(cls, array):
            """Build the matrix from a NumPy array (or any list-of-lists)."""
            return cls(array)

else:

    class _ListVector(list):
        """Fixed-length vector backed by :class:`list` (NumPy absent)."""

        _length: int = 0

        def __init__(self, iterable=None):
            if iterable is None:
                super().__init__([0.0] * self._length)
                return
            items = [float(x) for x in iterable]
            if self._length and len(items) != self._length:
                raise ValueError(
                    f"{type(self).__name__} must have exactly {self._length} elements"
                )
            super().__init__(items)

        def __array__(self, dtype=None, copy=None):
            # Only meaningful when NumPy is installed; provide a clear error
            # so the failure is obvious rather than a confusing AttributeError.
            raise TypeError("NumPy is not installed; cannot convert to array.")

        @classmethod
        def from_array(cls, array):
            """Build the vector from a list (or a NumPy array, if given one)."""
            if hasattr(array, "tolist"):
                return cls(array.tolist())
            return cls(array)

    class Vector1(_ListVector):
        """1D vector ``[x]`` (list-backed fallback)."""

        _length = 1

    class Vector2(_ListVector):
        """2D vector ``[x, y]`` (list-backed fallback)."""

        _length = 2

    class Vector3(_ListVector):
        """3D vector ``[x, y, z]`` (list-backed fallback)."""

        _length = 3

    class Matrix4x4(list):
        """4x4 transformation matrix backed by a list of lists (NumPy absent)."""

        def __init__(self, iterable=None):
            if iterable is None:
                super().__init__(
                    [
                        [1.0, 0.0, 0.0, 0.0],
                        [0.0, 1.0, 0.0, 0.0],
                        [0.0, 0.0, 1.0, 0.0],
                        [0.0, 0.0, 0.0, 1.0],
                    ]
                )
                return
            rows = [list(row) for row in iterable]
            if len(rows) != 4:
                raise ValueError("Matrix4x4 must have exactly 4 rows")
            cleaned: list[list[float]] = []
            for row in rows:
                if len(row) != 4:
                    raise ValueError("Each row in Matrix4x4 must have exactly 4 columns")
                cleaned.append([float(x) for x in row])
            super().__init__(cleaned)

        def __array__(self, dtype=None, copy=None):
            raise TypeError("NumPy is not installed; cannot convert to array.")

        @classmethod
        def from_array(cls, array):
            """Build the matrix from a list-of-lists (or a NumPy array)."""
            if hasattr(array, "tolist"):
                return cls(array.tolist())
            return cls(array)
