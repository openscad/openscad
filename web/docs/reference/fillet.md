# fillet

Add rounded fillets or chamfers to the edges of a solid. This is a PythonSCAD-specific function for edge treatment.

**Syntax:**

=== "Python"

```python
fillet(obj, r, sel=None, fn=2, minang=None)
obj.fillet(r, sel=None, fn=2, minang=None)
```

**Parameters:**

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `obj` | solid | — | The object to fillet |
| `r` | float | — | Fillet radius |
| `sel` | solid | `None` | Selection mask -- only edges intersecting this solid are filleted |
| `fn` | int | `2` | Number of segments. `fn=2` creates a bevel/chamfer; higher values create smoother rounds |
| `minang` | float | `None` | Minimum angle for edge selection |

**Examples:**

=== "Python"

```python
from openscad import *

c = cube(10)

# Bevel all edges (fn=2 = chamfer)
c.fillet(1).show()

# Smooth fillet with more segments
c.fillet(2, fn=5).show()

# Fillet only front-facing edges using a selection mask
mask = cube([30, 1, 30], center=True)
c.fillet(3, mask, fn=20).show()
```
