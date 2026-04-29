# Multi-tool Export

PythonSCAD ships a small helper, `MultiToolExporter`, for the common
multi-tool / multi-color workflow where a single model is split into several
parts (typically one per filament color or print head) and each part is
exported to its own file.

The class lives in the `pythonscad` package (a strict superset of `openscad`),
so it is available under either of these imports:

=== "Python"

```python
from pythonscad import *
# or, equivalently for this class:
from pythonscad import MultiToolExporter
```

## MultiToolExporter

`MultiToolExporter` is a `list` subclass whose items are
`(name, object)` 2-tuples. The `name` is a label used to build the output
filename and must be a non-empty string; output paths must be unique.
Item order matches `dict.items()` and the multi-object form of
[`export`](display.md#export), so a `MultiToolExporter` and a `dict` of
parts are interchangeable (`MultiToolExporter(..., items=parts.items())`
and `dict(exporter.parts())`).

**Filename layout:** for each item, the exporter writes to:

```text
{prefix}{name}{suffix}
```

so a typical use is `prefix="out/model-"` and `suffix=".stl"`.

**Cumulative-difference semantics:** for each index `i`, the geometry
exported is

```text
self[i].object − self[i+1].object − self[i+2].object − ... − self[-1].object
```

i.e. each entry's volume minus every later entry's volume.
This guarantees that overlapping regions are claimed by exactly one part:
**later entries "win" over earlier ones**, so each part only keeps the
volume not claimed by any subsequent part. The last entry is exported
as-is (no degenerate one-child `difference` node) and therefore claims
everything that overlaps with it.

**Constructor:**

=== "Python"

```python
MultiToolExporter(prefix, suffix, mkdir=False, items=())
```

**Parameters:**

| Parameter | Type                            | Default | Description                                                                            |
|-----------|---------------------------------|---------|----------------------------------------------------------------------------------------|
| `prefix`  | `str`                           | —       | Prepended to every output filename                                                     |
| `suffix`  | `str`                           | —       | Appended to every output filename (typically the extension, e.g. `".stl"`, `".3mf"`)   |
| `mkdir`   | `bool`                          | `False` | If `True`, create each output file's directory with `os.makedirs(..., exist_ok=True)`. See note below. |
| `items`   | iterable of `(name, object)`    | `()`    | Optional initial items (e.g. `a_dict.items()`), validated as if they were appended one at a time. |

When `mkdir=True`, filenames without a directory component (e.g.
`"flag-"` rather than `"out/flag-"`) are exported as-is - no directory
is created and no error is raised.

**Methods:**

| Method            | Description                                                                                                              |
|-------------------|--------------------------------------------------------------------------------------------------------------------------|
| `append(item)`    | Append a single `(name, object)` 2-tuple. Validates the shape.                                                           |
| `extend(items)`   | Append each `(name, object)` from an iterable.                                                                           |
| `insert(i, item)` | Insert a single `(name, object)` 2-tuple at position `i`.                                                                |
| `parts()`         | Return the computed `(name, geometry)` pairs in declaration order. Used internally by `export`/`show` and as input for `dict(exporter.parts())` to feed multi-object 3MF export. |
| `export()`        | Write each part to its file. Raises `ValueError` if any two items would write to the same output path.                  |
| `show()`          | Render each part into the preview viewport (same cumulative-difference semantics as `export`).                           |

**Validation:**

* Every inserted item must be a 2-tuple of `(str, object)`. Anything else
  raises `TypeError`.
* The `name` must be a non-empty string. Empty names raise `ValueError`.
* At `export()` time, the full output filename
  (`f"{prefix}{name}{suffix}"`) of every item is normalised with
  `os.path.normcase(os.path.normpath(filename))` (plus an extra
  `.casefold()` on macOS) and any collision raises `ValueError` rather
  than letting later parts silently overwrite earlier ones. This rejects
  raw duplicate names everywhere, plus path aliases like `"a/../b"` vs
  `"b"`, plus case-only collisions like `"a.stl"` vs `"A.stl"` on
  Windows and macOS (whose default filesystems are case-insensitive).
  Linux treats such case-only pairs as distinct files and is left
  alone.

**Examples:**

A two-color flag (red star cut out of a blue background):

=== "Python"

```python
from pythonscad import *

background = cube([200, 100, 1]).color("blue")
star       = cylinder(r=20, h=2, fn=5).translate([100, 50, -0.5]).color("red")

exporter = MultiToolExporter("out/flag-", ".stl", mkdir=True)
exporter.append(("blue", background))    # blue: rectangle minus the star area
exporter.append(("red",  star))          # red: the star itself (later wins)
exporter.export()
# -> writes out/flag-blue.stl and out/flag-red.stl
```

Seeding from the constructor:

=== "Python"

```python
from pythonscad import *

red  = cube(10).color("red")
blue = cube(10).color("blue").right(5)

MultiToolExporter(
    prefix="out/cube-",
    suffix=".3mf",
    mkdir=True,
    items=[("red", red), ("blue", blue)],
).export()
```

Previewing the same split inside the GUI without writing files:

=== "Python"

```python
from pythonscad import *

exporter = MultiToolExporter("ignored-", ".stl")
exporter.append(("red",  red))
exporter.append(("blue", blue))
exporter.show()
```

### Combining with multi-object 3MF export

`MultiToolExporter` writes one file per part. To combine its
cumulative-difference split with the
[multi-object 3MF form](display.md#multi-object-3mf-export) of
`export()` (one 3MF file containing every part as a named object),
run the parts through `parts()` and feed the result into a `dict`:

=== "Python"

```python
from pythonscad import *

background = cube([200, 100, 1]).color("blue")
star       = cylinder(r=20, h=2, fn=5).translate([100, 50, -0.5]).color("red")

exporter = MultiToolExporter("", "")  # prefix/suffix unused for this path
exporter.append(("blue", background))
exporter.append(("red",  star))

# Cumulative-difference split, then a single 3MF with two named parts.
export(dict(exporter.parts()), "flag.3mf")
```

The dict-form `export()` accepts only `.3mf` when, after filtering,
two or more recognised values remain. The filter is *not* a uniform
silent drop: `None` and `False` resolve to the built-in empty object
and `True` to the full universe (they pass through as named parts),
while truly unrelated values like numbers or strings are dropped, and
if nothing converts at all you get `TypeError: Object not recognized`.
Keys must be `str` (they become 3MF part names). See
[Multi-object 3MF export](display.md#multi-object-3mf-export) for the
full contract.
