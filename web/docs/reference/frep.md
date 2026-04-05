# F-Rep / libfive

PythonSCAD integrates [libfive](https://libfive.com/) for function representation (F-Rep) modeling using signed distance functions (SDFs). This feature requires PythonSCAD to be built with `-DENABLE_LIBFIVE=ON`.

## frep

Mesh a signed distance function (libfive expression) into a solid that PythonSCAD can display and manipulate.

**Syntax:**

=== "Python"

```python
frep(exp, min, max, res)
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `exp` | libfive tree | A libfive expression (SDF formula) |
| `min` | `[x, y, z]` | Minimum corner of the bounding box |
| `max` | `[x, y, z]` | Maximum corner of the bounding box |
| `res` | float | Resolution (higher = finer mesh) |

**Examples:**

=== "Python"

```python
from openscad import *
from pylibfive import *

c = lv_coord()
s1 = lv_sphere(lv_trans(c, [2, 2, 2]), 2)
b1 = lv_box(c, [2, 2, 2])
sdf = lv_union_stairs(s1, b1, 1, 3)

frep(sdf, [-4, -4, -4], [4, 4, 4], 20).show()
```

---

## ifrep

Convert a mesh (solid) into a libfive implicit function (SDF tree). This is the inverse of `frep` -- it takes a PythonSCAD solid and creates a libfive expression from it.

**Syntax:**

=== "Python"

```python
tree = ifrep(obj)
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `obj` | solid | A PythonSCAD solid to convert |

**Returns:** A libfive tree that can be used in SDF operations.

**Examples:**

=== "Python"

```python
from openscad import *

c = cube(5)
tree = ifrep(c)
```

---

## libfive Module

The `libfive` Python module provides low-level SDF building blocks. Import it with:

=== "Python"

```python
import libfive as lv
```

### Coordinate functions

| Function | Description |
|----------|-------------|
| `lv.x()` | X coordinate |
| `lv.y()` | Y coordinate |
| `lv.z()` | Z coordinate |

### Math functions

| Function | Description |
|----------|-------------|
| `lv.sqrt(x)` | Square root |
| `lv.square(x)` | Square (x^2) |
| `lv.abs(x)` | Absolute value |
| `lv.max(x, y)` | Maximum |
| `lv.min(x, y)` | Minimum |
| `lv.sin(x)` | Sine |
| `lv.cos(x)` | Cosine |
| `lv.tan(x)` | Tangent |
| `lv.asin(x)` | Arc sine |
| `lv.acos(x)` | Arc cosine |
| `lv.atan(x)` | Arc tangent |
| `lv.atan2(x, y)` | Two-argument arc tangent |
| `lv.exp(x)` | Exponential |
| `lv.log(x)` | Natural logarithm |
| `lv.pow(x, y)` | Power |
| `lv.comp(a, b)` | Comparison |
| `lv.print(formula)` | Print the expression tree |

### Operators

Libfive expressions support standard Python arithmetic operators (`+`, `-`, `*`, `/`, `%`) for building complex SDF formulas.

### Higher-level helpers (pylibfive)

The `pylibfive` library provides convenience functions built on top of the raw libfive module:

=== "Python"

```python
from pylibfive import *

c = lv_coord()
s = lv_sphere(c, 2)           # sphere SDF
b = lv_box(c, [2, 2, 2])      # box SDF
u = lv_union_stairs(s, b, 1, 3)  # staircase union
```

### Resources

- [Introduction to SDF (YouTube)](https://www.youtube.com/watch?v=62-pRVZuS5c&t=60s)
- [libfive documentation](https://libfive.com/)
