# I/O and Integration

## osimport

Import geometry from a file. This is the PythonSCAD equivalent of OpenSCAD's `import()` (renamed because `import` is a Python keyword).

**Syntax:**

=== "Python"

```python
osimport(file, layer="", convexity=2, origin=None, scale=None,
         width=0, height=0, center=False, dpi=96, id="", stroke=False,
         fn=0, fa=0, fs=0)
```

**Parameters:**

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `file` | string | — | Path to the file to import |
| `layer` | string | `""` | Layer name (for DXF files) |
| `convexity` | int | `2` | Convexity hint |
| `origin` | `[x, y]` | `None` | Origin offset (for DXF) |
| `scale` | float | `None` | Scale factor (for DXF) |
| `width` | float | `0` | Width (for image-based imports) |
| `height` | float | `0` | Height (for image-based imports) |
| `center` | bool | `False` | Center the imported geometry |
| `dpi` | float | `96` | DPI for SVG imports |
| `id` | string | `""` | Element ID (for SVG) |
| `stroke` | bool | `False` | Include stroke paths (for SVG) |
| `fn`, `fa`, `fs` | float | global | Curve discretization; defaults to the global `fn`/`fa`/`fs` values |

**Supported formats:** STL, OFF, AMF, 3MF (3D); DXF, SVG (2D)

**Examples:**

=== "Python"

```python
from openscad import *

model = osimport("model.stl")
model.show()

drawing = osimport("design.svg", dpi=96)
drawing.linear_extrude(height=2).show()
```

**OpenSCAD reference:** [import](https://en.wikibooks.org/wiki/OpenSCAD_User_Manual/Importing_Geometry#import)

---

## osuse

Load an OpenSCAD library file, making its modules and functions available. Equivalent to OpenSCAD's `use <file.scad>`.

**Syntax:**

=== "Python"

```python
osuse(file)
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `file` | string | Path to the `.scad` file |

**Examples:**

=== "Python"

```python
from openscad import *

osuse("MCAD/gears.scad")
```

---

## osinclude

Include an OpenSCAD file, executing its top-level code. Equivalent to OpenSCAD's `include <file.scad>`.

**Syntax:**

=== "Python"

```python
osinclude(file)
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `file` | string | Path to the `.scad` file |

**Examples:**

=== "Python"

```python
from openscad import *

osinclude("config.scad")
```

---

## scad

Execute inline OpenSCAD code from within a Python script.

**Syntax:**

=== "Python"

```python
scad(code)
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `code` | string | OpenSCAD source code to execute |

**Examples:**

=== "Python"

```python
from openscad import *

result = scad("cube(10);")
result.show()
```

---

## nimport

Import a model from a network URL. This function is only available in GUI mode.

**Syntax:**

=== "Python"

```python
nimport(url)
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `url` | string | URL of the model to download and import |

**Examples:**

=== "Python"

```python
from openscad import *

model = nimport("https://example.com/model.stl")
model.show()
```
