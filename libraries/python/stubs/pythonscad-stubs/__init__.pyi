"""Type stubs for the `pythonscad` package.

The `pythonscad` package is a strict superset of `openscad` (which itself
re-exports `_openscad`). PythonSCAD-only additions are surfaced here.
"""

# Convention (mirrors libraries/python/pythonscad/__init__.py): any
# import that is NOT part of the public `pythonscad` stub must be
# aliased with a leading underscore (e.g. ``import typing as _typing``).
# Type-checkers treat unaliased names in a stub as part of the public
# API surface, so leaking helpers here is just as bad as leaking them
# at runtime.
import typing as _typing

try:
    import numpy as _np
except ImportError:
    _np = _typing.Any
from openscad import *  # noqa: F401,F403
from openscad import (  # noqa: F401
    Color,
    PyLibFive,
    PyOpenSCAD,
    PyOpenSCADs,
)

HAS_NUMPY: bool

class _VectorBase(_np.ndarray[_typing.Any, _np.dtype[_np.float64]]):
    """Base class for NumPy-backed fixed-length PythonSCAD vectors."""

    def __init__(
        self, iterable: _typing.Iterable[float] | None = ...
    ) -> None: ...
    def __array__(
        self, dtype: _typing.Any = ..., copy: _typing.Any = ...
    ) -> _typing.Any: ...
    @classmethod
    def from_array(cls, array: _typing.Any) -> _typing.Self: ...

class Vector1(_VectorBase):
    """1D vector represented as [x]."""

class Vector2(_VectorBase):
    """2D vector represented as [x, y]."""

class Vector3(_VectorBase):
    """3D vector represented as [x, y, z]."""

class Matrix4x4(_np.ndarray[_typing.Any, _np.dtype[_np.float64]]):
    """NumPy-backed 4x4 transformation matrix helper."""

    def __init__(
        self,
        iterable: _typing.Iterable[_typing.Iterable[float]] | None = ...,
    ) -> None: ...
    def __array__(
        self, dtype: _typing.Any = ..., copy: _typing.Any = ...
    ) -> _typing.Any: ...
    @classmethod
    def from_array(cls, array: _typing.Any) -> "Matrix4x4": ...

class MultiToolExporter(list[tuple[str, _typing.Any]]):
    """List-based helper for exporting multi-tool / multi-color 3D models.

    Each item is a ``(name, object)`` 2-tuple (matching :func:`dict.items`
    and the multi-object form of :func:`export`). For each index ``i``,
    :meth:`export` writes the geometry obtained by subtracting every later
    item's object from ``self[i]``'s object into either per-part files
    named ``f"{prefix}{name}{suffix}"`` or one multi-object 3MF file when
    ``single_file`` is provided. The last entry is emitted as-is (no
    degenerate one-child ``difference`` node). Output paths and
    single-file part names must be unique; collisions raise
    :class:`ValueError` at export time.
    """

    prefix: str
    """String prepended to each output filename."""

    suffix: str
    """String appended to each output filename (typically the file extension)."""

    mkdir: bool
    """If True, create each output file's directory before exporting."""

    def __init__(
        self,
        prefix: str,
        suffix: str,
        mkdir: bool = ...,
        items: _typing.Iterable[tuple[str, _typing.Any]] = ...,
    ) -> None:
        """Initialize the exporter, optionally seeding it with ``items``."""
        ...

    def append(self, item: tuple[str, _typing.Any]) -> None:
        """Append a validated ``(name, object)`` tuple."""
        ...

    def extend(self, items: _typing.Iterable[tuple[str, _typing.Any]]) -> None:
        """Append each validated ``(name, object)`` tuple from ``items``."""
        ...

    def insert(
        self, index: _typing.SupportsIndex, item: tuple[str, _typing.Any]
    ) -> None:
        """Insert a validated ``(name, object)`` tuple at ``index``."""
        ...

    def __iadd__(  # type: ignore[override]
        self,
        other: _typing.Iterable[tuple[str, _typing.Any]],
    ) -> "MultiToolExporter":
        """Validate each item then in-place extend (``self += other``)."""
        ...

    def parts(self) -> list[tuple[str, _typing.Any]]:
        """Return computed ``(name, geometry)`` pairs in declaration order."""
        ...

    def export(self, single_file: str | None = ...) -> None:
        """Export each part separately, or all parts into one 3MF file."""
        ...

    def show(self) -> None:
        """Display each part in the PythonSCAD preview."""
        ...

@_typing.overload
def rounded_cube(
    size: float | Vector3,
    r: float,
    *,
    center: bool | None = ...,
    fn: float | None = ...,
    fa: float | None = ...,
    fs: float | None = ...,
) -> PyOpenSCAD: ...
@_typing.overload
def rounded_cube(
    size: float | Vector3,
    *,
    d: float,
    center: bool | None = ...,
    fn: float | None = ...,
    fa: float | None = ...,
    fs: float | None = ...,
) -> PyOpenSCAD: ...
def rounded_cube(
    size: float | Vector3,
    r: float | None = ...,
    *,
    d: float | None = ...,
    center: bool | None = ...,
    fn: float | None = ...,
    fa: float | None = ...,
    fs: float | None = ...,
) -> PyOpenSCAD:
    """Create a cube or box with uniformly rounded edges and corners.

    Specify exactly one of ``r`` (radius) or ``d`` (diameter). Set
    ``center=True`` to center the generated shape's bounding box on the origin;
    ``False`` or ``None`` leaves it in the positive octant. Optional ``fn``,
    ``fa``, and ``fs`` control rounding-sphere tessellation.
    """
    ...
