# Extrusions

Extrusion operations convert 2D shapes (or Python functions) into 3D solids.

## linear_extrude

Extrude a 2D shape (or Python function) along the Z axis.

**Syntax:**

=== "Python"

```python
linear_extrude(obj, height=1, convexity=1, origin=None, scale=None,
               center=False, slices=1, segments=0, twist=None)
obj.linear_extrude(height=1, ...)
```

**Parameters:**

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `obj` | 2D solid or function | — | Shape to extrude, or a Python function `f(h) -> [[x,y], ...]` |
| `height` | number or `[x,y,z]` | `1` | Extrusion height (number for Z-axis, vector for arbitrary direction) |
| `convexity` | int | `1` | Convexity for rendering |
| `origin` | `[x, y]` | `None` | Origin point for twist/scale operations |
| `scale` | `[x, y]` | `None` | Scale factor at the top of the extrusion |
| `center` | bool | `False` | Center vertically |
| `slices` | int | `1` | Number of intermediate slices |
| `segments` | int | `0` | Number of segments for twist |
| `twist` | number or function | `None` | Twist angle in degrees, or a Python function `f(h) -> angle` |
| `fn`, `fa`, `fs` | | — | Curve discretization parameters |

**PythonSCAD extensions:**

The `obj` parameter can be a Python function that receives a height value and returns a list of 2D points defining the cross-section at that height:

=== "Python"

```python
from openscad import *
from math import *

def xsection(h):
    v = 5 + sin(h)
    return [[-v, -v], [v, -v], [v, v], [-v, v]]

linear_extrude(xsection, height=10, fn=20).show()
```

The `twist` parameter can also be a Python function:

=== "Python"

```python
from openscad import *

square(5, center=True).linear_extrude(height=20, twist=90, slices=50).show()
```

**Examples:**

=== "Python"

```python
from openscad import *

circle(5).linear_extrude(height=10).show()

circle(5).linear_extrude(height=10, twist=90, slices=50).show()

circle(5).linear_extrude(height=10, scale=[2, 0.5]).show()
```

**OpenSCAD reference:** [linear_extrude](https://en.wikibooks.org/wiki/OpenSCAD_User_Manual/Using_the_2D_Subsystem#linear_extrude)

---

## rotate_extrude

Extrude a 2D shape by rotating it around the Z axis.

**Syntax:**

=== "Python"

```python
rotate_extrude(obj, convexity=1, scale=1.0, angle=360, twist=None,
               origin=None, offset=None, v=None, method=None)
obj.rotate_extrude(...)
```

**Parameters:**

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `obj` | 2D solid or function | — | Shape to extrude, or a Python function `f(angle) -> [[x,y], ...]` |
| `convexity` | int | `1` | Convexity for rendering |
| `scale` | float | `1.0` | Scale factor at the end of rotation |
| `angle` | float | `360` | Rotation angle in degrees |
| `twist` | number or function | `None` | Twist angle or function |
| `origin` | `[x, y]` | `None` | Origin for twist/scale |
| `offset` | `[x, y]` | `None` | Offset for the profile |
| `v` | `[x, y, z]` | `None` | Translation per revolution for helix (PythonSCAD extension) |
| `method` | string | `None` | Extrusion method |
| `fn`, `fa`, `fs` | | — | Curve discretization parameters |

**PythonSCAD extensions:**

The `obj` parameter can be a Python function that receives an angle and returns a 2D cross-section:

=== "Python"

```python
from openscad import *
from math import *

def xsection(h):
    v = 2 * sin(4 * pi * h)
    return [[10+v, -v], [15-v, -v], [15-v, 5+v], [10+v, 5+v]]

rotate_extrude(xsection, fn=50).show()
```

The `v` parameter creates a helix by translating the profile along a vector per revolution:

=== "Python"

```python
from openscad import *

circle(3).right(10).rotate_extrude(v=[0, 0, 20], angle=600).show()
```

**Examples:**

=== "Python"

```python
from openscad import *

# Simple torus
circle(3).right(10).rotate_extrude().show()

# Partial rotation
square([2, 3]).right(5).rotate_extrude(angle=270).show()
```

**OpenSCAD reference:** [rotate_extrude](https://en.wikibooks.org/wiki/OpenSCAD_User_Manual/Using_the_2D_Subsystem#rotate_extrude)

---

## path_extrude

Extrude a 2D shape along an arbitrary 3D path. A 4th value in each path vertex specifies the corner radius at that point.

**Syntax:**

=== "Python"

```python
path_extrude(obj, path, xdir=None, convexity=1, origin=None,
             scale=None, twist=None, closed=False, fn=-1, fa=-1, fs=-1)
obj.path_extrude(path, ...)
```

**Parameters:**

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `obj` | 2D solid or function | — | Cross-section shape or Python function |
| `path` | list of `[x,y,z]` or `[x,y,z,r]` | — | 3D path points; optional 4th element is corner radius |
| `xdir` | `[x, y, z]` | `[1, 0, 0]` | X-direction vector for profile orientation |
| `convexity` | int | `1` | Convexity for rendering |
| `origin` | `[x, y]` | `None` | Origin for the profile |
| `scale` | `[x, y]` | `None` | Scale factor at the end |
| `twist` | number or function | `None` | Twist angle or function |
| `closed` | bool | `False` | Close the path into a loop |
| `fn`, `fa`, `fs` | float | `-1` | Curve discretization |

**Examples:**

=== "Python"

```python
from openscad import *

# Extrude a square along a path with rounded corners
p = path_extrude(
    square(1),
    [[0,0,0], [0,0,10,3], [10,0,10,3], [10,10,10]]
)
p.show()
```

---

## skin

Create a surface that smoothly connects multiple 2D profiles placed in 3D space. This is essentially morphing between shapes.

**Syntax:**

=== "Python"

```python
skin(obj1, obj2, ..., convexity=1, align_angle=None, segments=None, interpolate=None)
```

**Parameters:**

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `obj1, obj2, ...` | 2D solids | — | Profiles to skin between (positioned in 3D space) |
| `convexity` | int | `1` | Convexity for rendering |
| `align_angle` | float | `None` | Angle to align profiles |
| `segments` | int | `None` | Number of interpolation segments |
| `interpolate` | float | `None` | Interpolation factor |

**Examples:**

=== "Python"

```python
from openscad import *

# Morph a square into a circle
a = square(4, center=True).roty(40)
b = circle(r=2, fn=20).rotx(40).up(10)
skin(a, b).show()
```
