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

from openscad import *  # noqa: F401,F403
from openscad import (  # noqa: F401
    Color,
    Matrix4x4,
    PyLibFive,
    PyOpenSCAD,
    PyOpenSCADs,
    Vector2,
    Vector3,
)


class MultiToolExporter(list[tuple[str, _typing.Any]]):
    """List-based helper for exporting multi-tool / multi-color 3D models.

    Each item is a ``(name, object)`` 2-tuple (matching :func:`dict.items`
    and the multi-object form of :func:`export`). For each index ``i``,
    :meth:`export` writes the geometry obtained by subtracting every later
    item's object from ``self[i]``'s object into the file
    ``f"{prefix}{name}{suffix}"``. The last entry is emitted as-is (no
    degenerate one-child ``difference`` node). Output paths must be
    unique; collisions (raw duplicate names, path aliases, or - on
    Windows/macOS - case-only collisions) raise :class:`ValueError` at
    export time.
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

    def insert(self, index: _typing.SupportsIndex, item: tuple[str, _typing.Any]) -> None:
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

    def export(self) -> None:
        """Export each part to a file via PythonSCAD."""
        ...

    def show(self) -> None:
        """Display each part in the PythonSCAD preview."""
        ...
