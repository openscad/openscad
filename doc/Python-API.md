# OpenSCAD Python API

OpenSCAD's experimental Python API builds a model from Python code. The embedded `openscad` module turns calls into OpenSCAD geometry; call `show()` with the final object (or list of objects) to set the model for preview and rendering.

```python
from openscad import cube, difference, show, sphere, union

body = union(
    sphere(r=18, fn=100),
    cube([20, 20, 8]).translate([0, 0, 15]),
)
cutout = cube([6, 24, 30]).translate([7, -2, -5])
show(difference(body, cutout))
```

## Objects and operations

Primitive functions include `square`, `circle`, `polygon`, `text`, `cube`, `cylinder`, `sphere`, and `polyhedron`. Most return an `OpenSCADObject`. Objects can be transformed with methods such as `.translate([x, y, z])`, `.rotate([x, y, z])`, `.scale(factor)`, `.mirror(vector)`, and `.color(color)`; these return new objects and leave the original unchanged.

The module also provides `union`, `difference`, `intersection`, `hull`, `minkowski`, `fill`, `linear_extrude`, `rotate_extrude`, `offset`, `resize`, `projection`, `surface`, `render`, and `align`. Many of these operations are available both as module functions and object methods. Python operators provide additional composition: `|` for union, `&` for intersection, `-` for difference, and `+` for translation by a vector.

## Preview modifiers

`highlight(obj)` or `obj.highlight()` applies the preview `#` modifier. `transparent(obj)` or `obj.transparent()` applies the preview `%` modifier. These affect preview display; they do not change the final rendered geometry.

`mesh(obj)` returns mesh data for an object. `textmetrics(...)` returns text measurement data. See [`openscad.pyi`](openscad.pyi) for function signatures, argument details, and return types.

## Running Python models

Python support is optional and disabled by default, so builds do not require Python development files. On macOS, the repository's `build.sh` explicitly enables Python. To configure a build manually, use:

```sh
cmake -S . -B build -DENABLE_PYTHON=ON
cmake --build build --target OpenSCADExe -j4
```

The Python-enabled build requires a Python interpreter and development headers/libraries for the target platform. In the GUI, enable **python-engine** under **Preferences → Features**, open a `.py` file, and trust it when prompted. Python scripts can execute arbitrary Python code, so only trust scripts from sources you trust.

For IDE imports and completions with a normal Python interpreter, install the placeholder package from the repository root:

```sh
python3 -m pip install -e ./python/ide-shim
```

The IDE shim is for editing and lightweight inspection only; its placeholder objects do not generate geometry. Run the script in OpenSCAD to preview or render the model.
