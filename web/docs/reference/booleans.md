# Boolean Operations

Boolean operations combine multiple solids using constructive solid geometry (CSG).

## union

Combine multiple objects into one. The result contains all volume from all inputs.

**Syntax:**

=== "Python"

```python
union(obj1, obj2, ..., r=None, fn=None)
obj1 | obj2
```

**Parameters:**

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `obj1, obj2, ...` | solids | — | Objects to combine |
| `r` | float | — | Fillet radius for newly created edges (PythonSCAD extension) |
| `fn` | int | — | Number of segments for fillet rounding (PythonSCAD extension) |

**PythonSCAD extensions:**

Specifying `r` and `fn` adds rounded fillets to the edges created by the union:

=== "Python"

```python
from openscad import *

union(cube(10), sphere(7).right(5), r=1, fn=10).show()
```

**Examples:**

=== "Python"

```python
from openscad import *

union(cube(10), sphere(7)).show()

# Operator form
(cube(10) | sphere(7)).show()
```

**OpenSCAD reference:** [union](https://en.wikibooks.org/wiki/OpenSCAD_User_Manual/CSG_Modelling#union)

---

## difference

Subtract one or more objects from the first object.

**Syntax:**

=== "Python"

```python
difference(obj1, obj2, ..., r=None, fn=None)
obj1 - obj2
```

**Parameters:**

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `obj1` | solid | — | Base object |
| `obj2, ...` | solids | — | Objects to subtract |
| `r` | float | — | Fillet radius for newly created edges (PythonSCAD extension) |
| `fn` | int | — | Number of segments for fillet rounding (PythonSCAD extension) |

**Examples:**

=== "Python"

```python
from openscad import *

difference(cube(10), sphere(7)).show()

# Operator form
(cube(10) - sphere(7)).show()

# With filleted edges
difference(cube(10), cylinder(r=4, h=12, center=True), r=0.5, fn=8).show()
```

**OpenSCAD reference:** [difference](https://en.wikibooks.org/wiki/OpenSCAD_User_Manual/CSG_Modelling#difference)

---

## intersection

Keep only the volume that is common to all input objects.

**Syntax:**

=== "Python"

```python
intersection(obj1, obj2, ...)
obj1 & obj2
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `obj1, obj2, ...` | solids | Objects to intersect |

**Examples:**

=== "Python"

```python
from openscad import *

intersection(cube(10), sphere(7)).show()

# Operator form
(cube(10) & sphere(7)).show()
```

**OpenSCAD reference:** [intersection](https://en.wikibooks.org/wiki/OpenSCAD_User_Manual/CSG_Modelling#intersection)

---

## hull

Create the convex hull of multiple objects. The result is the smallest convex solid that contains all input objects.

**Syntax:**

=== "Python"

```python
hull(obj1, obj2, ...)
obj1 ^ obj2
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `obj1, obj2, ...` | solids | Objects to hull |

**Examples:**

=== "Python"

```python
from openscad import *

hull(cube(3), sphere(2).right(10)).show()

# Operator form
(cube(3) ^ sphere(2).right(10)).show()
```

**OpenSCAD reference:** [hull](https://en.wikibooks.org/wiki/OpenSCAD_User_Manual/Transformations#hull)

---

## fill

Fill concavities in a 2D shape, creating a convex outline.

**Syntax:**

=== "Python"

```python
fill(obj1, obj2, ...)
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `obj1, obj2, ...` | 2D solids | Shapes to fill |

**Examples:**

=== "Python"

```python
from openscad import *

fill(polygon([[0,0], [10,0], [5,3], [10,10], [0,10]])).show()
```

---

## minkowski

Compute the Minkowski sum of two objects. Conceptually, this "traces" one object around the surface of the other, useful for rounding edges.

**Syntax:**

=== "Python"

```python
minkowski(obj1, obj2, convexity=2)
obj1 % obj2
```

**Parameters:**

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `obj1` | solid | — | Base object |
| `obj2` | solid | — | Object to trace around `obj1` |
| `convexity` | int | `2` | Convexity for rendering |

**Examples:**

=== "Python"

```python
from openscad import *

# Round the edges of a cube
minkowski(cube(10), sphere(1)).show()

# Operator form
(cube(10) % sphere(1)).show()
```

**OpenSCAD reference:** [minkowski](https://en.wikibooks.org/wiki/OpenSCAD_User_Manual/Transformations#minkowski)

---

## concat

Concatenate the meshes of multiple objects without performing boolean operations. This is useful when sub-parts are not yet watertight and CSG would fail.

**Syntax:**

=== "Python"

```python
concat(obj1, obj2, ...)
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `obj1, obj2, ...` | solids or lists | Objects to concatenate |

**Examples:**

=== "Python"

```python
from openscad import *

alltogether = concat(part1, part2, part3)
alltogether.show()
```
