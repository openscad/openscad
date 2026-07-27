#!/usr/bin/env python3
"""Unit tests for the ``pythonscad._vectors`` runtime vector/matrix helpers.

These exercise the pure-Python :class:`Vector1`/:class:`Vector2`/
:class:`Vector3` / :class:`Matrix4x4` classes that back
``from pythonscad import Vector3`` etc. The module is loaded directly by file
path (not via ``import pythonscad._vectors``) so that the package ``__init__``
-- and therefore the compiled ``_openscad`` extension -- is not required to
run these tests.

Both code paths are covered:

* NumPy installed  -> instances are ``numpy.ndarray`` subclasses.
* NumPy absent     -> instances are ``list`` subclasses. This path is
  exercised deterministically in a subprocess with ``numpy`` import blocked,
  regardless of whether NumPy is installed in the test environment.
"""
from __future__ import annotations

import importlib.util
import os
import subprocess
import sys
import textwrap
import unittest


_HERE = os.path.dirname(os.path.abspath(__file__))
_VECTORS_PY = os.path.join(
    _HERE, os.pardir, "libraries", "python", "pythonscad", "_vectors.py"
)


def _load_vectors():
    """Load ``pythonscad/_vectors.py`` standalone, without importing the package."""
    spec = importlib.util.spec_from_file_location(
        "_pythonscad_vectors_under_test", _VECTORS_PY
    )
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


ov = _load_vectors()


class CurrentBackingTests(unittest.TestCase):
    """Behaviour that must hold regardless of the active backing."""

    def test_construct_from_list(self):
        self.assertEqual(list(ov.Vector3([1, 2, 3])), [1.0, 2.0, 3.0])

    def test_construct_from_tuple(self):
        self.assertEqual(list(ov.Vector2((4, 5))), [4.0, 5.0])

    def test_construct_default_is_zero(self):
        self.assertEqual(list(ov.Vector3()), [0.0, 0.0, 0.0])

    def test_lengths(self):
        self.assertEqual(len(ov.Vector1([1])), 1)
        self.assertEqual(len(ov.Vector2([1, 2])), 2)
        self.assertEqual(len(ov.Vector3([1, 2, 3])), 3)

    def test_wrong_length_raises(self):
        with self.assertRaises(ValueError):
            ov.Vector3([1, 2])
        with self.assertRaises(ValueError):
            ov.Vector2([1, 2, 3])

    def test_matrix_identity_default(self):
        m = ov.Matrix4x4()
        rows = [list(r) for r in m]
        self.assertEqual(
            rows,
            [
                [1.0, 0.0, 0.0, 0.0],
                [0.0, 1.0, 0.0, 0.0],
                [0.0, 0.0, 1.0, 0.0],
                [0.0, 0.0, 0.0, 1.0],
            ],
        )

    def test_matrix_from_rows(self):
        src = [[1, 0, 0, 5], [0, 1, 0, 6], [0, 0, 1, 7], [0, 0, 0, 1]]
        m = ov.Matrix4x4(src)
        self.assertEqual([list(r) for r in m], [[float(x) for x in row] for row in src])

    def test_matrix_wrong_shape_raises(self):
        with self.assertRaises(ValueError):
            ov.Matrix4x4([[1, 2, 3]])
        with self.assertRaises(ValueError):
            ov.Matrix4x4([[1, 2, 3, 4]])  # only 1 row

    def test_from_array_roundtrip(self):
        v = ov.Vector3.from_array([9, 8, 7])
        self.assertEqual(list(v), [9.0, 8.0, 7.0])


@unittest.skipUnless(ov.HAS_NUMPY, "NumPy not installed")
class NumpyBackedTests(unittest.TestCase):
    """Behaviour specific to the NumPy-backed path."""

    def setUp(self):
        import numpy as np

        self.np = np

    def test_instances_are_ndarrays(self):
        self.assertIsInstance(ov.Vector3([1, 2, 3]), self.np.ndarray)
        self.assertIsInstance(ov.Matrix4x4(), self.np.ndarray)

    def test_construct_from_ndarray(self):
        v = ov.Vector3(self.np.array([1.0, 2.0, 3.0]))
        self.assertTrue(self.np.array_equal(v, [1.0, 2.0, 3.0]))

    def test_array_protocol(self):
        v = ov.Vector3([1, 2, 3])
        self.assertEqual(self.np.asarray(v).tolist(), [1.0, 2.0, 3.0])

    def test_vector_arithmetic(self):
        result = ov.Vector3([1, 2, 3]) + ov.Vector3([10, 20, 30])
        self.assertTrue(self.np.array_equal(result, [11.0, 22.0, 33.0]))

    def test_matrix_shape(self):
        self.assertEqual(ov.Matrix4x4().shape, (4, 4))

    def test_construct_does_not_alias_source(self):
        src = self.np.array([1.0, 2.0, 3.0])
        v = ov.Vector3(src)
        v[0] = 99.0
        self.assertEqual(src[0], 1.0)  # source must be untouched

    def test_iter_yields_python_floats(self):
        # list(v)/for-loops must yield plain floats, not numpy scalars, so
        # printed output matches the legacy types (see itertest golden).
        items = list(ov.Vector3([1, 2, 3]))
        self.assertEqual(items, [1.0, 2.0, 3.0])
        self.assertTrue(all(type(x) is float for x in items))

    def test_repr_is_list_form(self):
        self.assertEqual(repr(ov.Vector3([1, 2, 3])), "[1.0, 2.0, 3.0]")

    def test_eq_returns_plain_bool(self):
        v = ov.Vector3([1, 2, 3])
        self.assertIs(v == [1, 2, 3], True)
        self.assertIs(v == [9, 9, 9], False)
        self.assertIs(v != [9, 9, 9], True)
        self.assertFalse(v == 5)  # scalar -> NotImplemented -> False

    def test_unhashable(self):
        with self.assertRaises(TypeError):
            hash(ov.Vector3([1, 2, 3]))

    def test_matrix_repr_is_list_form(self):
        m = ov.Matrix4x4([[1.0, 0.0, 0.0, 5.0], [0.0, 1.0, 0.0, 0.0],
                          [0.0, 0.0, 1.0, 0.0], [0.0, 0.0, 0.0, 1.0]])
        self.assertEqual(
            repr(m),
            "[[1.0, 0.0, 0.0, 5.0], [0.0, 1.0, 0.0, 0.0], "
            "[0.0, 0.0, 1.0, 0.0], [0.0, 0.0, 0.0, 1.0]]",
        )


# Child-process program that forces the NumPy-absent (list-backed) path by
# blocking the numpy import before the module is loaded.
_NO_NUMPY_PROGRAM = textwrap.dedent(
    """
    import builtins, importlib.util, sys
    _orig = builtins.__import__
    def _blocked(name, *a, **k):
        if name == "numpy" or name.startswith("numpy."):
            raise ImportError("blocked for test")
        return _orig(name, *a, **k)
    builtins.__import__ = _blocked
    sys.modules.pop("numpy", None)
    _spec = importlib.util.spec_from_file_location("_ov_nonumpy", {vectors_py!r})
    ov = importlib.util.module_from_spec(_spec)
    _spec.loader.exec_module(ov)
    assert ov.HAS_NUMPY is False, "expected list-backed fallback"
    v = ov.Vector3([1, 2, 3])
    assert isinstance(v, list), type(v)
    assert v == [1.0, 2.0, 3.0], v
    assert isinstance(ov.Vector1([5]), list) and ov.Vector1([5]) == [5.0]
    assert list(ov.Vector2((4, 5))) == [4.0, 5.0]
    assert ov.Vector3() == [0.0, 0.0, 0.0]
    try:
        ov.Vector3([1, 2]); raise SystemExit("wrong-length accepted")
    except ValueError:
        pass
    m = ov.Matrix4x4()
    assert m == [
        [1.0, 0.0, 0.0, 0.0],
        [0.0, 1.0, 0.0, 0.0],
        [0.0, 0.0, 1.0, 0.0],
        [0.0, 0.0, 0.0, 1.0],
    ], m
    try:
        ov.Matrix4x4([[1, 2, 3]]); raise SystemExit("bad matrix accepted")
    except ValueError:
        pass
    try:
        ov.Vector3([1, 2, 3]).__array__(); raise SystemExit("array() should fail")
    except TypeError:
        pass
    # from_array accepts a plain list when numpy is missing ...
    assert ov.Vector3.from_array([7, 8, 9]) == [7.0, 8.0, 9.0]
    # ... and unwraps a numpy-like object that exposes tolist()
    class _Fake:
        def tolist(self):
            return [1, 2, 3]
    assert ov.Vector3.from_array(_Fake()) == [1.0, 2.0, 3.0]
    print("NO_NUMPY_OK")
    """
)


class ListBackedTests(unittest.TestCase):
    """The list-backed fallback, forced on via a numpy-blocked subprocess."""

    def test_fallback_behaviour(self):
        proc = subprocess.run(
            [sys.executable, "-c", _NO_NUMPY_PROGRAM.format(vectors_py=_VECTORS_PY)],
            capture_output=True,
            text=True,
        )
        self.assertEqual(proc.returncode, 0, msg=proc.stderr)
        self.assertIn("NO_NUMPY_OK", proc.stdout)


if __name__ == "__main__":
    unittest.main()
