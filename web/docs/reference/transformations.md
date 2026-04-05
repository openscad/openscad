# Transformations

All transformations are available both as standalone functions and as methods on solid objects.

## translate

Move an object by a displacement vector.

**Syntax:**

=== "Python"

```python
translate(obj, v)
obj.translate(v)
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `obj` | solid | The object to translate |
| `v` | `[x, y, z]` | Displacement vector |

**Examples:**

=== "Python"

```python
from openscad import *

cube(5).translate([10, 0, 0]).show()

translate(cube(5), [10, 20, 0]).show()
```

**OpenSCAD reference:** [translate](https://en.wikibooks.org/wiki/OpenSCAD_User_Manual/Transformations#translate)

---

## rotate

Rotate an object by angles in degrees.

**Syntax:**

=== "Python"

```python
rotate(obj, a, v=None, ref=None)
obj.rotate(a, v=None, ref=None)
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `obj` | solid | The object to rotate |
| `a` | number or `[x, y, z]` | Rotation angle(s) in degrees. A single number rotates around `v`; a list rotates around each axis |
| `v` | `[x, y, z]` | Axis of rotation (when `a` is a single number) |
| `ref` | `[x, y, z]` | Reference point for rotation |

**Examples:**

=== "Python"

```python
from openscad import *

cube(5).rotate([45, 0, 0]).show()

cube(5).rotate(45, [0, 0, 1]).show()
```

**OpenSCAD reference:** [rotate](https://en.wikibooks.org/wiki/OpenSCAD_User_Manual/Transformations#rotate)

---

## scale

Scale an object by factors along each axis.

**Syntax:**

=== "Python"

```python
scale(obj, v)
obj.scale(v)
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `obj` | solid | The object to scale |
| `v` | number or `[x, y, z]` | Scale factor(s) |

**Examples:**

=== "Python"

```python
from openscad import *

cube(5).scale([1, 2, 3]).show()
```

**OpenSCAD reference:** [scale](https://en.wikibooks.org/wiki/OpenSCAD_User_Manual/Transformations#scale)

---

## mirror

Mirror an object across a plane defined by a normal vector passing through the origin.

**Syntax:**

=== "Python"

```python
mirror(obj, v)
obj.mirror(v)
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `obj` | solid | The object to mirror |
| `v` | `[x, y, z]` | Normal vector of the mirror plane |

**Examples:**

=== "Python"

```python
from openscad import *

cube(5).mirror([1, 0, 0]).show()

cube(5).mirror([1, 1, 0]).show()
```

**OpenSCAD reference:** [mirror](https://en.wikibooks.org/wiki/OpenSCAD_User_Manual/Transformations#mirror)

---

## resize

Resize an object to fit exact dimensions.

**Syntax:**

=== "Python"

```python
resize(obj, newsize, auto=False, convexity=2)
obj.resize(newsize, auto=False, convexity=2)
```

**Parameters:**

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `obj` | solid | — | The object to resize |
| `newsize` | `[x, y, z]` | — | Target dimensions. Use `0` for an axis to keep its original size |
| `auto` | bool or `[bx, by, bz]` | `False` | Auto-scale axes with `0` size proportionally |
| `convexity` | int | `2` | Convexity for rendering |

**Examples:**

=== "Python"

```python
from openscad import *

sphere(5).resize([10, 10, 20]).show()

# Auto-scale: set X to 20, scale Y and Z proportionally
sphere(5).resize([20, 0, 0], auto=True).show()
```

**OpenSCAD reference:** [resize](https://en.wikibooks.org/wiki/OpenSCAD_User_Manual/Transformations#resize)

---

## multmatrix

Apply a 4x4 transformation matrix to an object.

**Syntax:**

=== "Python"

```python
multmatrix(obj, m)
obj.multmatrix(m)
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `obj` | solid | The object to transform |
| `m` | 4x4 list | Transformation matrix `[[a,b,c,d],[e,f,g,h],[i,j,k,l],[0,0,0,1]]` |

**Examples:**

=== "Python"

```python
from openscad import *

mat = [[1,0,0,10], [0,1,0,0], [0,0,1,0], [0,0,0,1]]
cube(5).multmatrix(mat).show()
```

**OpenSCAD reference:** [multmatrix](https://en.wikibooks.org/wiki/OpenSCAD_User_Manual/Transformations#multmatrix)

---

## divmatrix

Apply the inverse of a 4x4 transformation matrix. This is a PythonSCAD extension that simplifies undoing a `multmatrix` operation.

**Syntax:**

=== "Python"

```python
divmatrix(obj, m)
obj.divmatrix(m)
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `obj` | solid | The object to transform |
| `m` | 4x4 list | Transformation matrix whose inverse will be applied |

**Examples:**

=== "Python"

```python
from openscad import *

mat = [[1,0,0,10], [0,1,0,0], [0,0,1,0], [0,0,0,1]]
a = cube(1)
b = a.multmatrix(mat)   # move right by 10
c = b.divmatrix(mat)    # move back left
c.show()
```

---

## color

Apply a color to an object.

**Syntax:**

=== "Python"

```python
color(obj, c, alpha=1.0)
obj.color(c, alpha=1.0)
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `obj` | solid | The object to color |
| `c` | string or `[r, g, b]` or `[r, g, b, a]` | Color name (e.g. `"red"`, `"Tomato"`), hex string (`"#ff0000"`), or RGB/RGBA list (0.0-1.0) |
| `alpha` | float | Opacity (0.0 = transparent, 1.0 = opaque) |

**Examples:**

=== "Python"

```python
from openscad import *

cube(10).color("Tomato").show()

cube(10).color([1, 0, 0]).show()

cube(10).color("#ff6347").show()

cube(10).color("blue", alpha=0.5).show()
```

**OpenSCAD reference:** [color](https://en.wikibooks.org/wiki/OpenSCAD_User_Manual/Transformations#color)

---

## offset

Offset a 2D shape inward or outward.

**Syntax:**

=== "Python"

```python
offset(obj, r=None, delta=None, chamfer=False, fn=0, fa=0, fs=0)
obj.offset(r=None, delta=None, chamfer=False, fn=0, fa=0, fs=0)
```

**Parameters:**

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `obj` | 2D solid | — | The 2D shape to offset |
| `r` | float | `None` | Offset with rounded corners (positive = outward, negative = inward) |
| `delta` | float | `None` | Offset with sharp corners |
| `chamfer` | bool | `False` | Use chamfered corners instead of sharp (only with `delta`) |
| `fn`, `fa`, `fs` | float | global | Curve discretization; defaults to the global `fn`/`fa`/`fs` values |

Exactly one of `r` or `delta` should be provided.

**Examples:**

=== "Python"

```python
from openscad import *

square(10).offset(r=2).show()

square(10).offset(delta=-1).show()

square(10).offset(delta=2, chamfer=True).show()
```

**OpenSCAD reference:** [offset](https://en.wikibooks.org/wiki/OpenSCAD_User_Manual/Transformations#offset)

---

## Convenience Transforms

PythonSCAD provides shorthand functions for single-axis translations:

### right / left / front / back / up / down

**Syntax:**

=== "Python"

```python
right(obj, val)    # translate along +X
left(obj, val)     # translate along -X
front(obj, val)    # translate along -Y
back(obj, val)     # translate along +Y
up(obj, val)       # translate along +Z
down(obj, val)     # translate along -Z

obj.right(val)     # method form
```

**Examples:**

=== "Python"

```python
from openscad import *

cube(2).right(5).up(3).show()

cube(2).left(5).down(3).show()
```

---

## Convenience Rotations

Single-axis rotation shortcuts:

### rotx / roty / rotz

**Syntax:**

=== "Python"

```python
rotx(obj, val)     # rotate around X axis
roty(obj, val)     # rotate around Y axis
rotz(obj, val)     # rotate around Z axis

obj.rotx(val)      # method form
```

**Examples:**

=== "Python"

```python
from openscad import *

cube(5).rotx(45).show()

cube(5).roty(90).rotz(45).show()
```
