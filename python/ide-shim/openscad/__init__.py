"""IDE-only placeholder for OpenSCAD's embedded Python module.

This package supplies importable names and lightweight placeholder objects so
an IDE can run or inspect a model script with ordinary Python. It does not
create geometry or replace the module built into the OpenSCAD application.
"""

from __future__ import annotations

from typing import Any

__all__ = [
    "OpenSCADObject",
    "square",
    "circle",
    "polygon",
    "text",
    "textmetrics",
    "cube",
    "cylinder",
    "sphere",
    "polyhedron",
    "translate",
    "rotate",
    "scale",
    "mirror",
    "multmatrix",
    "divmatrix",
    "offset",
    "color",
    "show",
    "linear_extrude",
    "rotate_extrude",
    "union",
    "difference",
    "intersection",
    "hull",
    "minkowski",
    "fill",
    "resize",
    "projection",
    "surface",
    "mesh",
    "render",
    "align",
    "t",
]

t = 0.0


class OpenSCADObject:
    """A placeholder model object; operations return another placeholder."""

    def __init__(self, operation: str = "object", *args: Any, **kwargs: Any) -> None:
        self.operation = operation
        self.args = args
        self.kwargs = kwargs
        self.origin = [
            [1.0, 0.0, 0.0, 0.0],
            [0.0, 1.0, 0.0, 0.0],
            [0.0, 0.0, 1.0, 0.0],
            [0.0, 0.0, 0.0, 1.0],
        ]

    def __getattr__(self, name: str) -> Any:
        if name.startswith("_"):
            raise AttributeError(name)

        def apply(*args: Any, **kwargs: Any) -> OpenSCADObject:
            return OpenSCADObject(name, self, *args, **kwargs)

        return apply

    def __add__(self, other: Any) -> OpenSCADObject:
        return OpenSCADObject("union", self, other)

    def __or__(self, other: Any) -> OpenSCADObject:
        return OpenSCADObject("union", self, other)

    def __sub__(self, other: Any) -> OpenSCADObject:
        return OpenSCADObject("difference", self, other)

    def __and__(self, other: Any) -> OpenSCADObject:
        return OpenSCADObject("intersection", self, other)

    def __mul__(self, other: Any) -> OpenSCADObject:
        return OpenSCADObject("multiply", self, other)

    def __repr__(self) -> str:
        return f"OpenSCADObject({self.operation!r}, IDE shim; no geometry generated)"


def __getattr__(name: str) -> Any:
    if name not in __all__:
        raise AttributeError(f"module 'openscad' has no attribute {name!r}")
    if name == "show":
        def show(*args: Any, **kwargs: Any) -> None:
            return None

        return show
    if name == "textmetrics":
        def textmetrics(*args: Any, **kwargs: Any) -> dict[str, Any]:
            return {}

        return textmetrics
    if name == "mesh":
        def mesh(*args: Any, **kwargs: Any) -> tuple[list[Any], list[Any]]:
            return ([], [])

        return mesh

    def construct(*args: Any, **kwargs: Any) -> OpenSCADObject:
        return OpenSCADObject(name, *args, **kwargs)

    return construct
