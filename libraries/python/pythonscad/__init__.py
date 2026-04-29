"""PythonSCAD public API.

Strict superset of :mod:`openscad`. Currently a 1:1 re-export; future
PythonSCAD-only additions (new classes, helper APIs) will land here as
either symbols in this package or as submodules.

This package re-exports :mod:`openscad` rather than :mod:`_openscad`
directly, so any pure-Python implementation or override added in
:mod:`openscad` is automatically picked up.

The three-module layout is:

* :mod:`_openscad`  - C extension (low level, implementation detail).
* :mod:`openscad`   - ``_openscad`` + OpenSCAD-compatible pure-Python
  additions/overrides.
* :mod:`pythonscad` - this module: ``openscad`` + PythonSCAD-only
  extensions.

Switching a script between ``from openscad import *`` and
``from pythonscad import *`` requires no other code changes.
"""

from openscad import *  # noqa: F401,F403
from openscad import (  # noqa: F401
    ChildIterator,
    ChildRef,
    Openscad,
)

# Convention: any module-level import that is NOT meant to be part of
# the public `pythonscad` API must be aliased with a leading underscore
# (e.g. ``import os as _os``). Without the underscore the name is
# re-exported by ``from pythonscad import *``, leaks into IDE
# auto-complete, and shows up in the public stub. The unaliased
# re-exports above this line are deliberately public; everything below
# is internal helper machinery.
import os as _os
import sys as _sys
import typing as _typing


def _normalize_filename_key(filename: str) -> str:
    """Return a comparison key that matches what the OS uses for filenames.

    * :func:`os.path.normpath` collapses ``foo/../bar`` aliases and redundant
      separators on every OS.
    * :func:`os.path.normcase` lowercases and unifies separators on Windows;
      it is a no-op on POSIX.
    * On ``darwin`` we additionally apply :meth:`str.casefold` because the
      default macOS APFS volume is case-insensitive but :func:`os.path.normcase`
      is identity on POSIX. The trade-off is that case-sensitive APFS users
      see a false-positive collision rejection, which is the safer direction.

    On Linux the result is just ``os.path.normpath(filename)``, so two items
    differing only in case (``"a.stl"`` vs ``"A.stl"``) coexist as expected.
    """
    key = _os.path.normcase(_os.path.normpath(filename))
    if _sys.platform == "darwin":
        key = key.casefold()
    return key


class MultiToolExporter(list[tuple[str, _typing.Any]]):
    """List-based helper for exporting multi-tool / multi-color 3D models.

    Each item in the list is a ``(name, object)`` tuple, where ``name`` is a
    label used to build the output filename and ``object`` is a PythonSCAD
    geometry. The exporter is designed for workflows where a single model is
    split into several parts (for example, one part per filament color on a
    multi-tool 3D printer).

    Item order matches :func:`dict.items` and the multi-object form of
    :func:`export` (``export({"part1": obj1, "part2": obj2}, "out.3mf")``),
    so a :class:`MultiToolExporter` and a ``dict`` of parts are
    interchangeable: ``exporter = MultiToolExporter(..., items=parts.items())``
    and ``parts = dict(exporter.parts())``.

    Semantics
    ---------
    For each index ``i`` in the list, the exporter produces the geometry
    obtained by taking ``self[i]``'s object and subtracting every later
    item's object from it. Overlapping regions are therefore assigned to
    exactly one part: **later entries "win" over earlier ones**, so each
    part only keeps the volume not claimed by any subsequent part. The
    last entry is emitted as-is (no degenerate one-child ``difference``
    node) and therefore "wins" everything that overlaps with it.

    Attributes:
        prefix: String prepended to each output filename. Typically a path
            and/or base name, e.g. ``"out/model-"``.
        suffix: String appended to each output filename, typically the file
            extension, e.g. ``".stl"`` or ``".3mf"``.
        mkdir: If ``True``, the directory portion of each output filename is
            created (with :func:`os.makedirs`) before exporting. Defaults to
            ``False``. Filenames without a directory component are exported
            as-is, no error is raised.

    Validation
    ----------
    Items are validated on insertion (constructor argument, :meth:`append`,
    :meth:`extend`, :meth:`insert`, ``self[i] = ...``). A :class:`TypeError`
    is raised if the item is not a 2-tuple of ``(str, object)``, and a
    :class:`ValueError` is raised if the name is empty.

    Output paths must be unique per item: at :meth:`export` time, every
    item's full ``f"{prefix}{name}{suffix}"`` filename is normalised with
    :func:`os.path.normcase` and :func:`os.path.normpath` (plus
    :meth:`str.casefold` on macOS) and any collision raises
    :class:`ValueError`. This rejects raw duplicate names on every
    platform, plus path aliases such as ``"a/../b"`` vs ``"b"``, plus
    case-only collisions (``"a.stl"`` vs ``"A.stl"``) on Windows and
    macOS where the destination filesystem is typically case-insensitive.
    On Linux such pairs are accepted because the kernel treats them as
    distinct files.

    Example:
        >>> # Append base/background parts first; later entries "win" overlap.
        >>> exporter = MultiToolExporter("out/flag-", ".stl", mkdir=True)
        >>> exporter.append(("base", base_geometry))
        >>> exporter.append(("overlay", overlay_geometry))
        >>> exporter.export()  # writes out/flag-base.stl and out/flag-overlay.stl
    """

    def __init__(
        self,
        prefix: str,
        suffix: str,
        mkdir: bool = False,
        items: _typing.Iterable[tuple[str, _typing.Any]] = (),
    ):
        """Initialize a (possibly empty) MultiToolExporter.

        Args:
            prefix: String prepended to each output filename.
            suffix: String appended to each output filename, usually the file
                extension.
            mkdir: If ``True``, create the output directory for each file
                before exporting. Defaults to ``False``.
            items: Optional iterable of initial ``(name, object)`` tuples
                (e.g. ``a_dict.items()``). Each item is validated as if it
                were appended.

        Raises:
            TypeError: If any item in ``items`` is not a 2-tuple of
                ``(str, object)``.
            ValueError: If any name in ``items`` is empty.
        """
        super().__init__()
        self.prefix = prefix
        self.suffix = suffix
        self.mkdir = mkdir
        for item in items:
            self.append(item)

    @staticmethod
    def _validate_item(item: _typing.Any) -> tuple[str, _typing.Any]:
        """Return ``item`` if it is a valid ``(str, object)`` 2-tuple.

        Raises:
            TypeError: If ``item`` is not a 2-tuple whose first element is
                a string. Lists and other sequences are rejected.
            ValueError: If the name is empty.
        """
        if not isinstance(item, tuple) or len(item) != 2:
            raise TypeError(
                f"MultiToolExporter items must be (name, object) 2-tuples, "
                f"got {type(item).__name__}: {item!r}"
            )
        name, _ = item
        if not isinstance(name, str):
            raise TypeError(
                f"MultiToolExporter item name must be a str, "
                f"got {type(name).__name__}: {name!r}"
            )
        if not name:
            raise ValueError("MultiToolExporter item name must be a non-empty string")
        return item

    def append(self, item: tuple[str, _typing.Any]) -> None:
        """Append a validated ``(name, object)`` tuple."""
        super().append(self._validate_item(item))

    def extend(self, items: _typing.Iterable[tuple[str, _typing.Any]]) -> None:
        """Append each ``(name, object)`` tuple from ``items``.

        Atomic: every item is validated *before* anything is appended, so a
        bad item in the middle of the iterable leaves the exporter unchanged.
        """
        validated = [self._validate_item(item) for item in items]
        super().extend(validated)

    def insert(self, index: _typing.SupportsIndex, item: tuple[str, _typing.Any]) -> None:
        """Insert a validated ``(name, object)`` tuple at ``index``."""
        super().insert(index, self._validate_item(item))

    def __setitem__(self, index, value):
        """Replace one or more items, validating each new ``(name, object)``."""
        if isinstance(index, slice):
            super().__setitem__(index, [self._validate_item(v) for v in value])
        else:
            super().__setitem__(index, self._validate_item(value))

    def __iadd__(self, other):
        """Validate each item then in-place extend (``self += other``).

        Without this override, CPython's ``list.__iadd__`` calls
        ``list_extend`` at the C level and bypasses :meth:`extend`,
        which would let ``self += [bad_item]`` smuggle invalid entries
        past the validation hooks. Validation is atomic: every item in
        ``other`` is checked *before* anything is appended, so a bad
        item in the middle leaves ``self`` unchanged.
        """
        validated = [self._validate_item(item) for item in other]
        return super().__iadd__(validated)

    def _filename(self, i: int) -> str:
        """Return the output filename for part ``i``."""
        return f"{self.prefix}{self[i][0]}{self.suffix}"

    def _part(self, i: int):
        """Return the geometry for part ``i``: ``self[i]``'s object minus all later parts.

        For the last entry, the object is returned as-is rather than wrapped
        in a one-child ``difference`` node.
        """
        rest = [obj for _name, obj in self[i:]]
        return rest[0] if len(rest) == 1 else difference(*rest)  # noqa: F405

    def _check_unique_filenames(self) -> None:
        """Raise :class:`ValueError` if any two items resolve to the same output path.

        The dedup key is the full output filename
        (``f"{prefix}{name}{suffix}"``) normalised by
        :func:`_normalize_filename_key`, so the check rejects raw
        duplicate names plus path aliases (``"a/../b"`` vs ``"b"``) on
        every platform, plus case-only collisions (``"a.stl"`` vs
        ``"A.stl"``) on Windows and macOS where the destination
        filesystem is typically case-insensitive. On Linux such pairs
        are accepted as distinct files.
        """
        seen: dict[str, tuple[str, str]] = {}
        for i in range(len(self)):
            name = self[i][0]
            filename = self._filename(i)
            key = _normalize_filename_key(filename)
            if key in seen:
                prev_name, prev_filename = seen[key]
                raise ValueError(
                    f"MultiToolExporter items would write to the same output "
                    f"path: {prev_name!r} -> {prev_filename!r} and "
                    f"{name!r} -> {filename!r}"
                )
            seen[key] = (name, filename)

    def parts(self) -> list[tuple[str, _typing.Any]]:
        """Return the computed ``(name, geometry)`` pairs in declaration order.

        Each geometry is the cumulative-difference result identical to
        what :meth:`export` would write or :meth:`show` would preview:
        for index ``i``, ``self[i]``'s object minus every later item's
        object, with the last entry returned as-is (no degenerate
        one-child ``difference`` node).

        This is the programmatic accessor used internally by
        :meth:`export` and :meth:`show`. It is also the natural input
        to the multi-object 3MF export form
        (``export(dict(exporter.parts()), "out.3mf")``).

        Returns:
            A new list of ``(name, computed_geometry)`` tuples.
        """
        return [(self[i][0], self._part(i)) for i in range(len(self))]

    def export(self) -> None:
        """Export each part to a file via PythonSCAD.

        For each item, exports the result of :meth:`parts` to
        ``f"{prefix}{name}{suffix}"``.

        If :attr:`mkdir` is ``True``, the parent directory of each output
        file is created beforehand (filenames without a directory component
        are skipped silently).

        Raises:
            ValueError: If two or more items would write to the same
                output path (raw duplicate names, path aliases, or, on
                Windows/macOS, case-only collisions).
        """
        self._check_unique_filenames()
        for name, geometry in self.parts():
            filename = f"{self.prefix}{name}{self.suffix}"
            if self.mkdir:
                directory = _os.path.dirname(filename)
                if directory:
                    _os.makedirs(directory, exist_ok=True)
            export(geometry, filename)  # noqa: F405

    def show(self) -> None:
        """Display each part in the PythonSCAD preview.

        Iterates :meth:`parts` and passes each computed geometry to
        :func:`show`, producing a layered preview equivalent to what
        :meth:`export` would write to disk.
        """
        for _name, geometry in self.parts():
            show(geometry)  # noqa: F405
