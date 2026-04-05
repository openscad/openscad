# surface

Generate a 3D surface from a heightmap data file or image.

**Syntax:**

=== "Python"

```python
surface(file, center=False, convexity=2, invert=False, color=False)
```

**Parameters:**

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `file` | string | — | Path to a `.dat` (text heightmap) or `.png` (image heightmap) file |
| `center` | bool | `False` | Center the surface on the origin |
| `convexity` | int | `2` | Convexity for rendering |
| `invert` | bool | `False` | Invert the heightmap values |
| `color` | bool | `False` | Use color data from the image |

**File formats:**

- **DAT files:** Space-separated height values in a grid. Each line is a row, each value is the Z height at that grid point.
- **PNG files:** Pixel brightness is mapped to height.

**Examples:**

=== "Python"

```python
from openscad import *

surface(file="terrain.dat", center=True).show()

surface(file="heightmap.png", convexity=5).show()
```

**OpenSCAD reference:** [surface](https://en.wikibooks.org/wiki/OpenSCAD_User_Manual/Other_Language_Features#surface)
