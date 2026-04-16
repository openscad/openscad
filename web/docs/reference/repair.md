# Mesh Operations

## explode

Explode a solid outward by a vector, separating its components.

**Syntax:**

=== "Python"

```python
explode(obj, v)
obj.explode(v)
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `obj` | solid | The object to explode |
| `v` | `[x, y, z]` or number | Explosion vector or scalar |

**Examples:**

=== "Python"

```python
from openscad import *

cube(5).explode([1, 1, 1]).show()
```

---

## oversample

Subdivide mesh edges for finer geometric detail. This increases the number of triangles in the mesh.

**Syntax:**

=== "Python"

```python
oversample(obj, size, texture, projection, texturewidth, textureheight, texturedepth)
obj.oversample(size, texture, projection, texturewidth, textureheight, texturedepth)
```

**Parameters:**

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `obj`    | solid | — | The object to oversample |
| `size`   | double | — | maximal edge lengthl |
| `texture`| string | — | path to png filel |
| `projection`   | string | — | Projection type |
| `texturewidth`   | double | — | width of single canvas on model |
| `textureheigh`   | double | — | height of single danvas on model |
| `texturedepth`   | double | — | texture depth for maximal png color |


Valid projections are none, cylindrical, spherical, triplanar, cubic, planarx, planary, planarz

**Examples:**

=== "Python"

```python
from openscad import *

cube(5).oversample(2).show()
```

---

## debug

Visualize mesh faces for debugging purposes. Colors faces to help identify geometry issues.

**Syntax:**

=== "Python"

```python
debug(obj, faces=False)
obj.debug(faces=False)
```

**Parameters:**

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `obj` | solid | — | The object to debug |
| `faces` | bool | `False` | Show individual faces |

**Examples:**

=== "Python"

```python
from openscad import *

cube(5).debug().show()
```

---

## repair

Attempt to make a solid watertight (manifold). This is useful for fixing imported meshes that have holes, self-intersections, or other defects.

**Syntax:**

=== "Python"

```python
repair(obj, color=False)
obj.repair(color=False)
```

**Parameters:**

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `obj` | solid | — | The object to repair |
| `color` | bool | `False` | Preserve color information |

**Examples:**

=== "Python"

```python
from openscad import *

broken = osimport("broken_mesh.stl")
fixed = broken.repair()
fixed.show()
```

---

## separate

Split a solid into its disconnected components. Returns a list of separate solid objects.

**Syntax:**

=== "Python"

```python
separate(obj)
obj.separate()
```

**Parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `obj` | solid | The object to split |

**Returns:** A list of separate solid objects.

**Examples:**

=== "Python"

```python
from openscad import *

combined = concat(cube(5), cube(5).right(20))
parts = separate(combined)
for p in parts:
    p.show()
```
