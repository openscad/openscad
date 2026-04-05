# roof

Create a roof shape from a 2D polygon by lifting edges to form a peaked structure. This is an experimental feature that requires PythonSCAD to be built with both `ENABLE_EXPERIMENTAL` and `ENABLE_CGAL`.

**Syntax:**

=== "Python"

```python
roof(obj, method=None, convexity=2)
obj.roof(method=None, convexity=2)
```

**Parameters:**

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `obj` | 2D solid | — | The 2D polygon to create a roof from |
| `method` | string | `None` | Roof generation method |
| `convexity` | int | `2` | Convexity for rendering |

**Examples:**

=== "Python"

```python
from openscad import *

polygon([[0,0], [20,0], [20,20], [0,20]]).roof().show()
```
