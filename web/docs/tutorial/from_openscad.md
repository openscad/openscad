# Coming from OpenSCAD

PythonSCAD shares most function names and concepts with [OpenSCAD](https://openscad.org).
If you already know OpenSCAD, you can be productive quickly — with a few important
differences.

## Quick comparison

| Topic | OpenSCAD | PythonSCAD |
| --- | --- | --- |
| File type | `.scad` | `.py` (Python) |
| Displaying geometry | Shapes at top level appear automatically | Call `show(object)` |
| Imports | Built-in modules and `use` / `include` | `from pythonscad import *` (or `openscad`) |
| Language | OpenSCAD DSL | Python |

## Imports and modules

As with any Python library, bring symbols into scope with an import:

```python
from pythonscad import *
```

PythonSCAD ships two equivalent top-level modules:

- **`pythonscad`** — recommended for new PythonSCAD designs.
- **`openscad`** — for designs that should also run under upstream OpenSCAD's Python integration.

`pythonscad` is a strict superset of `openscad`, so switching between
`from pythonscad import *` and `from openscad import *` requires no other code change.

## Showing shapes

In OpenSCAD, a primitive at the top level is rendered automatically. In PythonSCAD,
store the solid in a variable and pass it to `show()`:

=== "Python"

    ```python
    from pythonscad import *

    c = cube([5, 5, 5])
    show(c)
    ```

=== "OpenSCAD"

    ```c++
    cube([5, 5, 5]);
    // displayed automatically
    ```

## Side-by-side syntax

Many tutorials use tabbed examples. Throughout the docs, **Python** tabs show
PythonSCAD; **OpenSCAD** tabs show the equivalent classic syntax for reference.

## Mixing OpenSCAD and Python

You can keep using `.scad` libraries and combine them with Python scripts.
See [Python together with OpenSCAD](./python_together.md).

## New to code CAD entirely?

If OpenSCAD is also new to you, start with the [Getting Started tutorial](./getting_started.md)
instead — it assumes no prior OpenSCAD knowledge.
