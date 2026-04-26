# PythonSCAD Python module layout

PythonSCAD ships three importable Python modules. They form a layered
public API: each layer is a strict superset of the layer below, so
switching a script between them never requires any other code change.

```text
+-----------------------------------------------------+
|  pythonscad   (PythonSCAD-only additions)           |
|  +-----------------------------------------------+  |
|  |  openscad   (OpenSCAD-compatible API)         |  |
|  |  +-----------------------------------------+  |  |
|  |  |  _openscad   (C extension, low level)   |  |  |
|  |  +-----------------------------------------+  |  |
|  +-----------------------------------------------+  |
+-----------------------------------------------------+
```

## What lives where

### `_openscad` — C extension

- Built from `src/python/` (registered in the embedded interpreter via
  `PyImport_AppendInittab("_openscad", &PyInit__openscad)`; shipped as
  `_openscad.so` / `_openscad.pyd` in the pip wheel).
- Implementation detail. User scripts should import `pythonscad` or
  `openscad` instead.
- The leading underscore follows the standard CPython convention for
  private extension modules backing a higher-level Python package.

### `openscad` — OpenSCAD-compatible overlay

- Pure-Python package at
  [`libraries/python/openscad/__init__.py`](../libraries/python/openscad/__init__.py).
- Re-exports `_openscad` 1:1 today.
- Kept compatible with upstream OpenSCAD's Python integration: any
  design written against `from openscad import *` should remain runnable
  on both upstream OpenSCAD (where available) and PythonSCAD.
- This is also the place where pure-Python reimplementations or
  compatibility shims for OpenSCAD-compatible APIs go.

### `pythonscad` — PythonSCAD superset

- Pure-Python package at
  [`libraries/python/pythonscad/__init__.py`](../libraries/python/pythonscad/__init__.py).
- Re-exports `openscad` (not `_openscad` directly) and is the home for
  PythonSCAD-only features that don't make sense in upstream OpenSCAD.
- Recommended import for new PythonSCAD designs.

Because `pythonscad` re-exports `openscad`, any pure-Python override
added in `openscad` is automatically picked up by `pythonscad` users.

## Drop-in property

For every public name `n` shipped today:

```python
import _openscad, openscad, pythonscad
assert openscad.n is _openscad.n
assert pythonscad.n is openscad.n
```

This is asserted by the regression test
`tests/data/pythonscad-overlay-echo/overlay-identity.py` and by the pip
smoke test in `scripts/smoke-test-pip.py`.

## Per-symbol deprecation pattern (future-ready)

The plan is **not** to deprecate the `openscad` module as a whole. Some
PythonSCAD features have been merged into upstream OpenSCAD and using
`openscad` keeps designs portable. PythonSCAD has, however, added much
more functionality on top, and at some point it may be useful to nudge
users away from PythonSCAD-only symbols that happen to be exposed under
the `openscad` name.

`libraries/python/openscad/__init__.py` exposes a `_deprecated()` helper
for that purpose. To deprecate a single symbol `foo` on the `openscad`
overlay (without touching `_openscad` or `pythonscad`):

```python
# in libraries/python/openscad/__init__.py
foo = _deprecated("foo", replacement="foo")(_openscad.foo)
```

Because `pythonscad` re-exports `openscad`, this would normally also
wrap the symbol for `pythonscad` users. To keep the un-wrapped version
on `pythonscad`, rebind it explicitly:

```python
# in libraries/python/pythonscad/__init__.py, after the `from openscad import *`
from _openscad import foo as foo
```

This recipe keeps the default case (no deprecations, the file you read
above) trivial and contains any special-casing in a small explicit
override list.

## How CPython finds the overlays

In the **embedded interpreter** (the `pythonscad` GUI/CLI binary):

- `initPython` in `src/python/pyopenscad.cc` adds
  `<resourceBasePath>/libraries/python` to `sys.path` (plus a few
  development/install fallbacks). The `openscad/` and `pythonscad/`
  packages there get picked up automatically.
- `_openscad` is registered via `PyImport_AppendInittab` and resolves
  as a built-in.

In a **pip-installed wheel**:

- The wheel contains an `_openscad.<ext>` extension plus two pure-Python
  packages `openscad/` and `pythonscad/`, all dropped into
  `site-packages` by setuptools. Standard import resolution handles the
  rest.

## Re-init cleanup (embedded interpreter only)

`initPython` clears user-imported modules from `sys.modules` between
script runs so a stale `from openscad import foo` in script A does not
pollute script B. The cleanup explicitly drops `openscad` and
`pythonscad` (in addition to whatever the file-based filter catches),
but intentionally **does not** drop `_openscad`: the C extension is a
process-wide singleton whose internal state must not be torn down.
